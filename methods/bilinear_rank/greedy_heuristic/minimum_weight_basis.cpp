#include "minimum_weight_basis.h"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "device.h"
#include "gf2_bits.h"
#include "gf2_span_walk.h"
#include "memory_budget.h"
#include "measures.h"
#include "reflected_gray_walk.h"
#include "span_basis.h"
#include "span_enumeration.h"
#include "span_ranks_on_card.h"

namespace bilinear_rank {

std::vector<Matrix> minimum_weight_basis_with(const Field& field, const std::vector<Matrix>& slices,
                               const Matrix& candidate, const std::vector<std::size_t>& known,
                               std::size_t* cost) {
    std::vector<Matrix> enlarged = slices;
    enlarged.push_back(candidate);
    return minimum_weight_basis(field, enlarged, known, cost);
}

namespace {

class GreedyBasis;

/// The span walked one slice at a time, with the index carried alongside.
///
/// Both loops in this file used to rebuild element `index` from
/// `coefficient_vector(index)`, which is `O(k · width)` multiply-accumulates for
/// a thing that is `O(width)` field additions away from the element before it:
/// under an order where consecutive strings differ in one digit by one, which is
/// what [`ReflectedGrayWalk`](reflected_gray_walk.h) is. This is that walk with
/// the base-`p` value of the string maintained beside it, `± p^digit` a step, so
/// a caller still knows which slot it is standing on.
///
/// **The order is invisible to both callers**, and that is the whole reason this
/// is safe: one writes `ranks[index]`, the other sorts by `(rank, index)`. See
/// [`minimum_weight_basis.h`](minimum_weight_basis.h) on where it would not be.
///
/// This is the general field's representation of the span, one `int64_t` an
/// entry, and it is what runs over GF(3), GF(5) and every shape too wide to
/// pack. [`gf2_span_walk.h`](gf2_span_walk.h) is the same span over GF(2) in
/// bits, offering the same five questions, and the two functions at the bottom
/// of this file choose between them once a call.
class SpanElements {
   public:
    using Greedy = GreedyBasis;

    SpanElements(const Field& field, const std::vector<Matrix>& slices)
        : field_(field),
          slices_(slices),
          radix_(static_cast<std::size_t>(field.characteristic())),
          place_(slices.size()),
          walk_(slices.size(), radix_) {
        // The all-zero combination, which is the string the walk starts on.
        linear_combination_into(field_, slices_, std::vector<int64_t>(slices_.size(), 0), element_);
        width_ = element_.entry_count();
        for (std::size_t position = 0, weight = 1; position < slices_.size(); ++position) {
            place_[position] = weight;
            weight *= radix_;
        }
    }

    /// Step to the next element, or say the span is used up. The index of the
    /// element now in `at()` is `index()`.
    bool advance() {
        ReflectedGrayWalk::Step step;
        if (!walk_.advance(step)) return false;

        // One digit moved by one, so one slice goes in or comes out. No
        // multiplication, and no rebuild.
        const Element* slice = slices_[step.digit].data();
        Element* entries = element_.data();
        if (step.upward) {
            index_ += place_[step.digit];
            for (std::size_t entry = 0; entry < width_; ++entry) {
                field_.addin(entries[entry], slice[entry]);
            }
        } else {
            index_ -= place_[step.digit];
            for (std::size_t entry = 0; entry < width_; ++entry) {
                field_.subin(entries[entry], slice[entry]);
            }
        }
        return true;
    }

    std::size_t index() const { return index_; }
    const Matrix& at() const { return element_; }

    /// The rank of the element the walk stands on, which is what the whole
    /// enumeration is for.
    std::size_t rank() const { return linear_algebra::rank(field_, element_); }

    /// The rank of one slice: what the ceiling and the floor are made of.
    std::size_t rank_of_slice(std::size_t slice) const {
        return linear_algebra::rank(field_, slices_[slice]);
    }

    /// How many elements a basis of this span holds, which is where the greedy
    /// stops.
    std::size_t dimension() const { return linear_algebra::span_of(field_, slices_).dimension(); }

    /// What the greedy is built from, so it need not be handed the same two
    /// things the walk already holds.
    const Field& field() const { return field_; }
    const std::vector<Matrix>& slices() const { return slices_; }

   private:
    const Field& field_;
    const std::vector<Matrix>& slices_;
    std::size_t radix_;
    std::size_t width_ = 0;
    /// `p^position`, so the index is carried rather than recovered.
    std::vector<std::size_t> place_;
    ReflectedGrayWalk walk_;
    Matrix element_;
    std::size_t index_ = 0;
};

/// The basis the greedy is assembling, over field elements.
///
/// Only the rank and the index of a candidate are held while the span is
/// walked, so the element is rebuilt here from its index when the greedy
/// actually reaches it. `Gf2GreedyBasis` is the same thing in bits.
class GreedyBasis {
   public:
    explicit GreedyBasis(const SpanElements& walk)
        : field_(walk.field()),
          slices_(walk.slices()),
          span_(walk.field(), linear_algebra::flattened_width<Field>(walk.slices())) {}

    /// The element at `index`, taken if it is outside what is already held.
    bool take(std::size_t index, Matrix& element) {
        element = linear_combination(
            field_, slices_, coefficient_vector(index, slices_.size(), field_.characteristic()));
        return span_.try_add(element);
    }

   private:
    const Field& field_;
    const std::vector<Matrix>& slices_;
    ReducedBasis span_;
};

/// The largest rank the answer is allowed to hold, from the slices alone.
///
/// The greedy basis is not merely cheapest in total: sorted by rank it is
/// **dominated position by position** by every other basis. For each `t` the
/// number of its elements of rank at most `t` is the dimension of the span of
/// everything in the span that cheap, `[oxley, Lem. 1.8.3]`, and no independent
/// set can hold more members that cheap than that dimension. So the `i`-th
/// cheapest element of the answer is no dearer than the `i`-th cheapest of any
/// other basis, and in particular its **dearest** element is no dearer than the
/// dearest element of any basis drawn from `slices` themselves, of which there
/// is one, since a spanning set contains a basis.
///
/// That is the whole licence for the three lines in the walk below that drop an
/// element instead of ranking it. An element above this ceiling cannot enter the
/// answer, and one that cannot enter the answer cannot change it either: the
/// greedy stops the moment the basis fills, so it never reaches past the dearest
/// element it took.
///
/// It costs one rank per slice and saves one per span element above it. On
/// `cyclic_f2_7` that is 12 ranks a call to skip 1 856 of them.
template <class Elements>
std::size_t highest_rank_the_answer_can_hold(const Elements& walk, std::size_t slices) {
    std::size_t ceiling = 0;
    for (std::size_t slice = 0; slice < slices; ++slice) {
        ceiling = std::max(ceiling, walk.rank_of_slice(slice));
    }
    return ceiling;
}

/// A floor under the rank of a span element nobody has ranked yet.
///
/// The elements this is asked about are the ones `ranks_without_last` does not
/// cover: those whose coefficient `c` on the last slice `g` is not zero. Such an
/// element is `v + c·g`, where `v` is the element of the span without `g` at
/// `index % p^k` and its rank is already held. Rank is subadditive and
/// `rank(c·g) == rank(g)` for every `c != 0`, so
///
///     rank(v + c·g) >= rank(v) - rank(g)   and   rank(v + c·g) >= rank(g) - rank(v),
///
/// which together are `|rank(v) - rank(g)|`: one subtraction against the
/// Gaussian elimination the true rank costs.
///
/// **Zero, and not one, is the floor where the two ranks agree.** `v + c·g` can
/// be the zero matrix, whose rank is zero, and a floor above a true rank is
/// exactly what would make dropping an element unsound.
struct RankFloor {
    /// The ranks of the span without the last slice, read by index.
    const std::vector<std::size_t>& known;
    /// `p^k`, or zero where there is nothing to read and no floor to give.
    std::size_t period = 0;
    /// The rank of the slice the last coefficient multiplies.
    std::size_t added = 0;

    std::size_t under(std::size_t index) const {
        if (period == 0) return 0;
        const std::size_t held = known[index % period];
        return held > added ? held - added : added - held;
    }
};

/// The floor, where the ranks handed over really are the span without the last
/// slice.
///
/// A caller may hand over any prefix it likes and the walk believes it only for
/// the indices it covers, so a vector of another length reads here as "no floor"
/// rather than as an error. The one rank this costs is paid once a call.
template <class Elements>
RankFloor floor_under_the_unranked(const Field& field, const Elements& walk, std::size_t slices,
                                   const std::vector<std::size_t>& ranks_without_last) {
    if (slices == 0 || ranks_without_last.empty()) return {ranks_without_last};
    if (ranks_without_last.size() != span_size(field, slices - 1)) {
        return {ranks_without_last};
    }
    return {ranks_without_last, ranks_without_last.size(), walk.rank_of_slice(slices - 1)};
}

/// The same ranks from a card, or false and nothing written.
///
/// **Five reasons to decline, and none of them is a failure.** No backend was
/// registered, which is every build without `nvcc`; a field that is not GF(2),
/// where the bit-packed arithmetic a kernel does is not this arithmetic; a shape
/// no kernel was compiled for; a span too small to be worth a launch, which
/// [`../../../infrastructure/run_limits/device.h`](../../../infrastructure/run_limits/device.h) decides and not this
/// function; or the backend itself declining. On this machine the fourth is what
/// almost always answers, and that is the correct answer:
/// [`span_ranks_on_card.h`](span_ranks_on_card.h) says why the seam exists
/// anyway.
///
/// **The launch floor is asked here rather than inside the backend**, for the
/// reason `Gf2Leaf` asks it rather than letting a leaf backend decide: one place
/// decides where work goes, for the host too.
bool ranks_from_a_card(const Field& field, const std::vector<Matrix>& slices,
                       std::size_t combinations, std::vector<std::size_t>& ranks) {
    const SpanRanksOnCard* card = span_ranks_on_card();
    if (card == nullptr || slices.empty()) return false;
    if (field.characteristic() != 2) return false;

    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();
    if (!card->handles(rows, columns)) return false;
    if (run_limits::chosen_device(combinations) != run_limits::Device::Gpu) return false;

    // The slices in the order they were given, because over GF(2) that order is
    // the index: element `i` is the exclusive or of the slices whose bit is set
    // in `i`.
    const std::size_t width = rows * columns;
    const std::size_t words = linear_algebra::gf2_word_count(width);
    std::vector<std::uint64_t> packed(slices.size() * words);
    for (std::size_t slice = 0; slice < slices.size(); ++slice) {
        linear_algebra::gf2_pack(slices[slice].data(), width, &packed[slice * words]);
    }

    PackedSpan span;
    span.rows = rows;
    span.columns = columns;
    span.words = words;
    span.slices = slices.size();
    span.slice_rows = packed.data();
    return card->ranks(span, combinations, ranks);
}

/// Every element of the span ranked into its own slot, whatever the walk is
/// made of. The walk starts on index 0, the zero combination, whose rank is
/// zero in either representation.
template <class Elements>
void every_rank_into(Elements& walk, std::vector<std::size_t>& ranks) {
    ranks[0] = walk.rank();
    while (walk.advance()) {
        ranks[walk.index()] = walk.rank();
    }
}

// The elements of the span that could enter a basis, cheapest first. Index 0
// is the zero combination and is skipped: it can never enter one.
//
// Only the rank and the index are held. The element itself is rebuilt from
// its index by the greedy when it actually reaches it, which is at
// most `dimension` times. Holding the matrices instead costs
// `p^slices * (56 + 8*n*m)` bytes: 134 MB for the sixteen slices of 4x4
// matrix multiplication, against 1 MB this way.
struct Candidate {
    std::size_t rank;
    std::size_t index;
};

/// The whole of step 1, over whichever representation the walk is made of.
///
/// **Every line of the policy is here and none of it is in the walk**: the
/// ceiling, the floor, which elements are pushed, the sort, and the order the
/// greedy takes them in. A packed run and a general run execute this one
/// function, and differ only in what a rank and a membership test are made of,
/// which is what makes "the same answer" a property of the code rather than a
/// claim about two copies of it.
template <class Elements>
std::vector<Matrix> basis_walked_over(const Field& field, const std::vector<Matrix>& slices,
                                      const std::vector<std::size_t>& ranks_without_last,
                                      Elements& walk, std::size_t* cost) {
    const std::size_t dimension = walk.dimension();
    const std::size_t combinations = span_size(field, slices.size());

    // The whole span, and not what the walk keeps: this is the budget the
    // enumeration is allowed to ask for, and a run refused before is refused
    // still. Nothing is reserved against it, because the ceiling below leaves a
    // fraction of the span standing and the reserve would be the largest
    // allocation in the function for room it never fills.
    run_limits::require_room("the span of " + std::to_string(slices.size()) + " slices",
                 combinations - 1, sizeof(Candidate));

    std::vector<Candidate> candidates;
    // The same Gray walk `span_element_ranks` takes, for the same saving: the
    // element is one slice away from the one before it rather than `k`
    // multiply-accumulates away from its own index. The walk starts on the
    // all-zero string, which is index 0 and is skipped here as it was before,
    // and visits every other index exactly once, so this pushes the same
    // `combinations - 1` candidates.
    //
    // **The elements a `ranks_without_last` covers are still not ranked**, which
    // is the whole of that optimisation; they are stepped over rather than
    // skipped, because the walk is what carries the element to the ones that
    // are. One pass over the entries against the `k` the rebuild cost.
    //
    // **What the ceiling drops, it drops for good**, and that is what makes it
    // free rather than a trade: an element the answer cannot hold changes
    // neither the basis nor its cost, so it need not be ranked, sorted or
    // rebuilt. The floor decides that for the unranked half without looking at
    // the matrix at all.
    const std::size_t ceiling = highest_rank_the_answer_can_hold(walk, slices.size());
    const RankFloor floor = floor_under_the_unranked(field, walk, slices.size(), ranks_without_last);

    while (walk.advance()) {
        const std::size_t index = walk.index();
        if (index < ranks_without_last.size()) {
            if (ranks_without_last[index] > ceiling) continue;
            candidates.push_back({ranks_without_last[index], index});
            continue;
        }
        if (floor.under(index) > ceiling) continue;
        const std::size_t rank = walk.rank();
        if (rank > ceiling) continue;
        candidates.push_back({rank, index});
    }

    // Sort by rank, ties broken by enumeration order, to ensure reproducible
    // results. **That is a total order on distinct indices**, so the sequence
    // the walk pushed them in cannot reach this answer: the sorted vector is the
    // one the index-order loop produced, entry for entry.
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& left, const Candidate& right) {
                  if (left.rank != right.rank) return left.rank < right.rank;
                  return left.index < right.index;
              });

    std::vector<Matrix> basis;
    std::size_t weight = 0;
    typename Elements::Greedy greedy(walk);
    Matrix element;
    for (const Candidate& candidate : candidates) {
        if (basis.size() == dimension) break;
        if (!greedy.take(candidate.index, element)) continue;
        // The rank of what was just taken, already computed above: the sum
        // of these is what `multiplication_count` would recover from the
        // matrices, one Gaussian elimination per basis element at a time
        // when the answer is already known.
        weight += candidate.rank;
        basis.push_back(std::move(element));
    }

    // The ceiling is a claim about what the answer holds, and a basis that came
    // up short under it would compute a different map rather than compute the
    // same one slower: the one failure nothing downstream would catch, since
    // every count taken from it would be lower and look better. Checked here,
    // where the claim is spent, and not only in the test that holds the filtered
    // walk against the unfiltered one.
    if (basis.size() != dimension) {
        throw std::runtime_error("a minimum-weight basis of " + std::to_string(dimension) +
                                 " dimensions came out holding " + std::to_string(basis.size()) +
                                 ": the rank ceiling dropped an element the answer needed");
    }
    if (cost != nullptr) *cost = weight;
    return basis;
}

}  // namespace

std::vector<std::size_t> span_element_ranks(const Field& field,
                                            const std::vector<Matrix>& slices) {
    const std::size_t combinations = span_size(field, slices.size());
    run_limits::require_room("the ranks of a span of " + std::to_string(slices.size()) + " slices",
                 combinations, sizeof(std::size_t));

    std::vector<std::size_t> ranks(combinations);
    if (ranks_from_a_card(field, slices, combinations, ranks)) return ranks;

    // Over GF(2) the same walk in bits, which is the same order over the same
    // span and writes the same slots: [`gf2_span_walk.h`](gf2_span_walk.h).
    if (gf2_span_walk_applies(field, slices)) {
        Gf2SpanElements walk(field, slices);
        every_rank_into(walk, ranks);
        return ranks;
    }

    SpanElements walk(field, slices);
    every_rank_into(walk, ranks);
    return ranks;
}

std::vector<Matrix> minimum_weight_basis(const Field& field, const std::vector<Matrix>& slices,
                                   const std::vector<std::size_t>& ranks_without_last,
                                   std::size_t* cost) {
    if (gf2_span_walk_applies(field, slices)) {
        Gf2SpanElements walk(field, slices);
        return basis_walked_over(field, slices, ranks_without_last, walk, cost);
    }

    SpanElements walk(field, slices);
    return basis_walked_over(field, slices, ranks_without_last, walk, cost);
}

}  // namespace bilinear_rank
