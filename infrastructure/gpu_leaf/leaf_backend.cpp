#include "leaf_backend.h"

#include <algorithm>
#include <exception>
#include <mutex>
#include <vector>

#include "gpu_leaf.h"

namespace gpu_leaf {

namespace {

/// One leaf on the card at a time, whatever `--threads` says.
///
/// **The span lives in `__constant__` memory and there is one of it.** Two
/// workers routing leaves to the card at once would upload two spans to the same
/// symbol and each would reduce against the other's: a wrong survivor set, and
/// so a wrong rank-one basis, with nothing downstream to catch it. The shape
/// tables below are shared for the same reason.
///
/// A card is one resource, so serialising costs nothing that was ever available:
/// what the workers keep in parallel is everything above the leaf.
std::mutex the_card;

/// The four shapes both `.cu` files instantiate a kernel at, in the one place a
/// caller may ask. Their `launch` switches throw on anything else, which is a
/// backstop and not the check: a search must be able to ask *before* it commits
/// a leaf, and an exception is not an answer to "can you".
bool handles(std::size_t rows, std::size_t columns) {
    if (rows != columns) return false;
    return rows == 4 || rows == 5 || rows == 9 || rows == 16;
}

/// The ceilings the two kernels hold their span in, from their own constants.
/// A leaf past either is declined, which is the host's job and not news.
constexpr std::size_t kWordCeiling = 4;
constexpr std::size_t kScanDimensionCeiling = 256;
constexpr std::size_t kWalkDimensionCeiling = 64;

/// Survivors one launch may report before the range has to be cut. 512 KB of
/// device memory, allocated per launch, which is why it is not the 32 MB the
/// harness uses: a leaf is asked thousands of times in a search and a harness
/// four times in a run.
///
/// Not `constexpr`, because [`tests/test_survivor_overflow.cpp`](tests/test_survivor_overflow.cpp)
/// shrinks it. Overflow is the one branch here that no question in this
/// repository reaches (a 47-dimensional span keeps a handful of the four
/// billion maps tested against it), and a branch nothing exercises is a branch
/// that does not work.
std::size_t survivor_capacity = 1u << 16;

/// Halvings allowed before the leaf is handed back to the host.
///
/// Bounded rather than unbounded, because the recovery is for something that
/// does not happen and thrashing through sixteen halvings would cost more than
/// the host would have. Six is at most 64 launches, and past it the host
/// answers, which is always correct and is never a truncation.
constexpr int kHalvingsAllowed = 6;

/// The two vector lists in the kernel's width, kept between leaves.
///
/// They are a fact about the shape and not about the leaf, and a search asks
/// thousands of leaves of one shape. Converting 65 535 masks a side at 16x16 is
/// half a megabyte of copying that would otherwise happen once per leaf, against
/// a launch this whole file exists to keep under a hundred microseconds.
struct ShapeTables {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::vector<std::uint32_t> lefts;
    std::vector<std::uint32_t> rights;
};

/// `Gf2Leaf` carries a vector as a `uint64_t` and the kernels as a `uint32_t`.
/// Narrowed with a cast written out rather than left implicit: no shape with a
/// kernel is wider than sixteen columns, and a shape that ever were would be
/// silently truncated here rather than refused.
void narrowed(const std::uint64_t* masks, std::size_t count, std::vector<std::uint32_t>& into) {
    into.resize(count);
    for (std::size_t index = 0; index < count; ++index) {
        into[index] = static_cast<std::uint32_t>(masks[index]);
    }
}

const ShapeTables& tables_for(const bilinear_rank::PackedLeaf& leaf) {
    static ShapeTables held;
    if (held.rows == leaf.rows && held.columns == leaf.columns &&
        held.lefts.size() == leaf.left_count && held.rights.size() == leaf.right_count) {
        return held;
    }
    held.rows = leaf.rows;
    held.columns = leaf.columns;
    narrowed(leaf.left_masks, leaf.left_count, held.lefts);
    narrowed(leaf.right_masks, leaf.right_count, held.rights);
    return held;
}

/// The leaf as the kernels read it. The span is copied because it changes at
/// every leaf and is two kilobytes; the mask tables are copied from the cache
/// above, which is where the size is.
LeafQuestion question_from(const bilinear_rank::PackedLeaf& leaf, bool needs_the_grid) {
    LeafQuestion question;
    question.rows = leaf.rows;
    question.columns = leaf.columns;
    question.width = leaf.rows * leaf.columns;
    question.words = leaf.words;
    question.left_count = leaf.left_count;
    question.right_count = leaf.right_count;
    if (needs_the_grid) {
        const ShapeTables& held = tables_for(leaf);
        question.left_masks = held.lefts;
        question.right_masks = held.rights;
    }
    question.span_rows.assign(leaf.span_rows, leaf.span_rows + leaf.dimension * leaf.words);
    question.pivots.assign(leaf.pivots, leaf.pivots + leaf.dimension);
    return question;
}

/// Every survivor of `[begin, end)`, in bites small enough that one launch's
/// buffer holds each one's.
///
/// **`overflowed` means the indices are not an answer**, not that they are a
/// short one: the threads past the capacity never wrote, and which of them those
/// were is a race between blocks. So an overflowed bite is thrown away whole and
/// the range halved, which halves the survivors expected of it, and nothing here
/// ever reads an index from a launch that said it overflowed.
///
/// The bites are ascending and disjoint, so what comes back is ascending, which
/// is what the host's greedy needs to keep the maps the sequential loop kept.
///
/// Past `kHalvingsAllowed` the whole leaf is refused rather than truncated, and
/// the host answers it.
template <typename Launch>
bool every_survivor(std::uint64_t begin, std::uint64_t end, const Launch& launch,
                    std::vector<std::uint64_t>& survivors) {
    std::uint64_t bite = end > begin ? end - begin : 1;
    int halvings = 0;
    std::vector<std::uint64_t> found;
    for (std::uint64_t first = begin; first < end;) {
        const GpuSurvivors reported = launch(first, std::min(end, first + bite));
        if (reported.overflowed) {
            if (halvings == kHalvingsAllowed) return false;
            bite = bite / 2 == 0 ? 1 : bite / 2;
            ++halvings;
            continue;  // the same first element, a smaller bite of it
        }
        found.insert(found.end(), reported.indices.begin(), reported.indices.end());
        first = std::min(end, first + bite);
    }
    // Written only on the way out, so a caller handed false never sees a partial
    // list it could mistake for a short one.
    survivors = std::move(found);
    return true;
}

/// Everything below this line may throw, and none of it may throw at the search.
///
/// `cuda_guard.cuh` turns every failed call into an exception naming the call,
/// which is what a measurement wants. A search wants the host to answer and the
/// run to say so afterwards, because a card that failed is a slower run and
/// never a wrong one.
template <typename Answer>
bool answered_or_the_host(const Answer& answer) try {
    return answer();
} catch (const std::exception& failure) {
    bilinear_rank::note_card_failure(failure.what());
    return false;
}

bool scan_on_the_card(const bilinear_rank::PackedLeaf& leaf, std::size_t left_rows,
                      std::vector<std::uint64_t>& survivors) {
    if (leaf.words > kWordCeiling || leaf.dimension > kScanDimensionCeiling) return false;
    if (leaf.left_masks == nullptr || leaf.right_masks == nullptr) return false;

    const std::lock_guard<std::mutex> only_one(the_card);
    return answered_or_the_host([&] {
        const LeafQuestion question = question_from(leaf, true);
        std::vector<std::uint64_t> found;
        const bool whole = every_survivor(
            0, left_rows,
            [&](std::uint64_t first, std::uint64_t last) {
                return scan_pool_on_gpu(question, static_cast<std::size_t>(first),
                                        static_cast<std::size_t>(last), survivor_capacity);
            },
            found);
        if (!whole) return false;
        survivors = std::move(found);
        return true;
    });
}

bool walk_on_the_card(const bilinear_rank::PackedLeaf& leaf, std::uint64_t elements,
                      std::vector<std::uint64_t>& survivors) {
    if (leaf.words > kWordCeiling || leaf.dimension > kWalkDimensionCeiling) return false;

    const std::lock_guard<std::mutex> only_one(the_card);
    return answered_or_the_host([&] {
        const LeafQuestion question = question_from(leaf, false);
        std::vector<std::uint64_t> found;
        // Index zero is the zero map, which the host's walk does not examine
        // either, so the range starts at one on both sides.
        const bool whole = every_survivor(
            1, elements,
            [&](std::uint64_t first, std::uint64_t last) {
                return walk_subspace_on_gpu(question, first, last, survivor_capacity);
            },
            found);
        if (!whole) return false;
        survivors = std::move(found);
        return true;
    });
}

const bilinear_rank::LeafOnCard offered{&handles, &scan_on_the_card, &walk_on_the_card};

}  // namespace

const bilinear_rank::LeafOnCard& card_backend() { return offered; }

void set_survivor_capacity(std::size_t capacity) { survivor_capacity = capacity; }

}  // namespace gpu_leaf
