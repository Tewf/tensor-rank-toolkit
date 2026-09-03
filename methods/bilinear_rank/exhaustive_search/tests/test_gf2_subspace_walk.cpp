/// The two walks of one subspace are the same answer, map for map.
///
/// [`gf2_leaf.h`](../gf2_leaf.h) answers a leaf's subspace walk two ways.
/// `by_rebuilding_each_element` forms every element from the binary digits of
/// its index; `by_walking_the_subspace` carries one running combination along a
/// reflected Gray code, where an element costs a single exclusive or, and drains
/// each batch of the walk back into index order. **The second is required to be
/// the first's answer and not an equivalent one**: the same maps, in the same
/// order, because `Gf2SpanBasis::try_add` is order-dependent, so which rank-one
/// basis a leaf hands back is a fact about the order it saw its candidates in,
/// and every decomposition this repository prints is a stack of those.
///
/// This is the demand the general-field pair in
/// [`test_gray_walk_leaf.cpp`](test_gray_walk_leaf.cpp) does *not* make of
/// itself: there the Gray order reaches the greedy and the two routes are held
/// only to the same count, which is the leaf's verdict. Here the order is put
/// back before the greedy sees anything, so the stronger claim is available and
/// is what gets asserted.
///
/// A disagreement would mostly not crash. Gray order is not index order, so a
/// greedy fed the survivors as the walk finds them returns a different basis of
/// the same space and every count still matches. Batches drained out of order
/// (which is what cutting one global Gray walk into fixed runs would give, since
/// run `m` of such a walk covers the index block `G(m)`) would do the same,
/// and only on the spans whose survivors straddle a boundary. A budget spent at
/// a different rate would abandon a leaf somewhere else and turn a refutation
/// into an undecided, or the reverse. So the two are run against each other on
/// many spans and compared as maps, and the budget is asserted to stop them at
/// the same element.
///
/// **Over the three widths the repository searches at**: 4x4 from GF(2^4)
/// multiplication, 5x5 from the polynomial fixture, and the 9x9 slices of
/// `<3,3,3>`. The dimensions run to 14, which is well past the 8 digits one
/// batch varies, so most of the spans here cross batch boundaries and the ones
/// below 9 pin the single-batch case beside them.
///
/// Neither routine reads the pool: a walked subspace touches it only for the
/// shape, so the leaf is built once per shape and with no table, rather than
/// once per storage route the way the pool scan has to be.
#include <cstddef>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "exhaustive_search.h"
#include "gf2_leaf.h"
#include "memory_budget.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Addressed;
using bilinear_rank::Field;
using bilinear_rank::Gf2Leaf;
using bilinear_rank::Matrix;
using bilinear_rank::RankOnePool;
using bilinear_rank::ReducedBasis;
using bilinear_rank::SearchBudget;

/// Same maps, same order. Not the same count: the count is the leaf's verdict
/// and it survives any order, which is exactly what this may not settle for.
bool same(const std::vector<Matrix>& left, const std::vector<Matrix>& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index].rows() != right[index].rows()) return false;
        if (left[index].columns() != right[index].columns()) return false;
        for (std::size_t entry = 0; entry < left[index].entry_count(); ++entry) {
            if (left[index].data()[entry] != right[index].data()[entry]) return false;
        }
    }
    return true;
}

/// Hundreds of comparisons reported as one line, with the first disagreement by
/// name so that a failure says where it was and not only that there was one.
/// The maps are counted as well as compared, because two routines that both
/// returned nothing would agree on every span and settle nothing.
struct Tally {
    long long compared = 0;
    long long disagreed = 0;
    long long maps = 0;
    std::string first;

    void saw(bool agreed, long long returned, const std::string& what) {
        ++compared;
        maps += returned;
        if (agreed) return;
        if (disagreed == 0) first = what;
        ++disagreed;
    }

    void report(const std::string& label, long long expected) {
        check::equal(label + ": spans compared", compared, expected);
        check::equal(label + ": and the two walks agreed on every one", disagreed, 0);
        if (disagreed != 0) std::cout << "        first disagreement at " << first << "\n";
        check::equal(label + ": rank-one maps returned, rather than nothing to compare",
                     maps > 0 ? 1 : 0, 1);
    }
};

std::size_t elements_of(const ReducedBasis& span) { return std::size_t(1) << span.dimension(); }

void compare(const std::string& what, const Gf2Leaf<Addressed>& leaf, const ReducedBasis& span,
             std::size_t needed, std::size_t elements, Tally& tally) {
    const std::vector<Matrix> walked = leaf.by_walking_the_subspace(span, needed, elements);
    const std::vector<Matrix> rebuilt = leaf.by_rebuilding_each_element(span, needed, elements);
    tally.saw(same(walked, rebuilt), static_cast<long long>(walked.size()), what);
}

/// The spans every shape is asked about: the tensor's own, one slice at a time,
/// and spans built out of pool elements, which have a rank-one basis by
/// construction and so are the only ones where a walk returns much at all on a
/// product shape.
std::vector<std::pair<std::string, ReducedBasis>> spans_to_ask_about(
    const Field& field, const RankOnePool& pool, const std::vector<Matrix>& slices,
    std::size_t width) {
    std::vector<std::pair<std::string, ReducedBasis>> spans;

    ReducedBasis growing(field, width);
    for (std::size_t index = 0; index < slices.size(); ++index) {
        growing.try_add(slices[index]);
        spans.emplace_back("span of " + std::to_string(index + 1) + " slices", growing);
    }

    // Fixed seed: a test that walks a different span each run cannot be
    // reported, and the point here is not to sample the space but to have a
    // dozen spans nobody chose to make either route look right.
    std::mt19937_64 generator(20260820);
    for (std::size_t dimension = 1; dimension <= 14 && dimension <= width; ++dimension) {
        ReducedBasis span(field, width);
        while (span.dimension() < dimension) span.try_add(pool.at(generator() % pool.size()));
        spans.emplace_back("random rank-one span of dimension " + std::to_string(dimension),
                           std::move(span));
    }
    return spans;
}

/// Every target worth asking for, reachable and not.
///
/// `0` never stops the walk, so it visits every element and takes every
/// independent rank-one map in the span. `1` stops at the first survivor, which
/// is the early return inside a batch. The dimension is the leaf's own question,
/// reachable exactly when the span has a rank-one basis. The two above it cannot
/// be reached at all.
std::vector<std::size_t> targets_for(const ReducedBasis& span, std::size_t width) {
    return {0, 1, span.dimension(), span.dimension() + 1, width + 1};
}

/// Counts of elements that are not `2 ^ dim`, where the last batch runs past the
/// end and has to be drained short.
///
/// The walk is cut into batches of a fixed power of two, so a count that is not
/// one leaves a final batch the Gray order covers whole and the drain must stop
/// inside. Nothing in the search passes such a count (`rank_one_basis.cpp`
/// computes `p ^ dim` exactly), but the routine takes one, and the two routines
/// have to agree wherever it is legal rather than only where it is reached.
std::vector<std::size_t> partial_counts(std::size_t elements) {
    return {1, 2, elements / 3, elements - 1};
}

/// A budget stops both walks at the same element.
///
/// Asked with a target no walk can reach, so nothing ends it but the subspace or
/// the limit: `may_examine` is then called once per index from 1 upward, and the
/// first refusal is at index `limit`. So a limit below the element count must
/// abandon the leaf and a limit at it must not, on both routes, and the maps
/// collected before the stop must still match.
void compare_under_a_budget(const std::string& what, const Gf2Leaf<Addressed>& leaf,
                            const ReducedBasis& span, std::size_t width, Tally& tally) {
    const std::size_t elements = elements_of(span);
    for (const std::size_t limit : {std::size_t(1), elements / 3, elements - 1, elements}) {
        SearchBudget for_walk(/*limit=*/1'000'000, /*leaf_limit=*/limit);
        SearchBudget for_rebuild(/*limit=*/1'000'000, /*leaf_limit=*/limit);
        const std::vector<Matrix> walked =
            leaf.by_walking_the_subspace(span, width + 1, elements, &for_walk);
        const std::vector<Matrix> rebuilt =
            leaf.by_rebuilding_each_element(span, width + 1, elements, &for_rebuild);

        const bool expected = limit < elements;
        const bool agreed = same(walked, rebuilt)
                            && for_walk.leaf_abandoned.load() == expected
                            && for_rebuild.leaf_abandoned.load() == expected
                            && for_walk.tree_fully_walked.load() == for_rebuild.tree_fully_walked.load();
        tally.saw(agreed, static_cast<long long>(walked.size()),
                  what + " under a leaf limit of " + std::to_string(limit));
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_gf2_subspace_walk <fixtures>\n";
        return 1;
    }
    const std::string directory = argv[1];

    // 4x4, 5x5 and 9x9, which are the widths this repository searches at.
    for (const char* name : {"gf16_multiplication", "f2_5x5", "matmul_3x3x3"}) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + name + ".tensor");
        const Field field(tensor.characteristic);
        const std::size_t rows = tensor.rows();
        const std::size_t columns = tensor.columns();
        const std::size_t width = rows * columns;
        const std::string shape = std::to_string(rows) + "x" + std::to_string(columns);
        const std::string label = std::string(name) + " (" + shape + ")";

        const RankOnePool addressed(field, rows, columns);
        const Addressed pool{addressed};
        const auto spans = spans_to_ask_about(field, addressed, tensor.slices, width);

        // No table: a walked subspace reads the pool for nothing but its shape,
        // so building one would price a route neither routine takes.
        run_limits::set_memory_budget(1);
        const Gf2Leaf<Addressed> leaf(field, pool, rows, columns);
        run_limits::set_memory_budget(std::size_t(2) << 30);

        Tally over_the_whole_subspace;
        for (const auto& span : spans) {
            const std::size_t elements = elements_of(span.second);
            for (const std::size_t needed : targets_for(span.second, width)) {
                compare(label + ", " + span.first + ", target " + std::to_string(needed), leaf,
                        span.second, needed, elements, over_the_whole_subspace);
            }
        }
        over_the_whole_subspace.report(label, static_cast<long long>(spans.size() * 5));

        // The same spans stopped short of `2 ^ dim`, which is the only way a
        // batch ever ends inside the drain rather than at its own boundary.
        Tally cut_short;
        for (const auto& span : spans) {
            if (span.second.dimension() < 2) continue;
            for (const std::size_t elements : partial_counts(elements_of(span.second))) {
                compare(label + ", " + span.first + ", first " + std::to_string(elements)
                            + " elements",
                        leaf, span.second, 0, elements, cut_short);
                compare(label + ", " + span.first + ", first " + std::to_string(elements)
                            + " elements, target 1",
                        leaf, span.second, 1, elements, cut_short);
            }
        }

        long long expected_cuts = 0;
        for (const auto& span : spans) {
            if (span.second.dimension() >= 2) expected_cuts += 8;
        }
        cut_short.report(label + ", stopped short of the whole subspace", expected_cuts);

        // And under a budget, on one span either side of a batch boundary: at
        // dimension 6 the whole walk is one batch, and at 11 a limit of a third
        // of the elements refuses partway through the ninth of eight-and-a-bit.
        Tally bounded;
        for (const auto& span : spans) {
            if (span.first != "random rank-one span of dimension 6"
                && span.first != "random rank-one span of dimension 11") {
                continue;
            }
            compare_under_a_budget(label + ", " + span.first, leaf, span.second, width, bounded);
        }
        bounded.report(label + ", under a budget", 8);
    }

    return check::report("gf2 subspace walk");
}
