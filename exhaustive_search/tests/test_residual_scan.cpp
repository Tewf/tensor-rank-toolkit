/// The two scans of one pool are the same answer, map for map.
///
/// [`gf2_leaf.h`](../gf2_leaf.h) answers a leaf's pool scan two ways.
/// `by_scanning_the_pool_directly` asks the span about every element in turn;
/// `by_carrying_a_residual` never forms an element and never asks, carrying a
/// reduction along a Gray code over the right-hand vectors instead. **The second
/// is required to be the first's answer and not an equivalent one** — the same
/// maps, in the same order — because `Gf2SpanBasis::try_add` is order-dependent,
/// so which rank-one basis a leaf hands back is a fact about the order it saw
/// its candidates in, and every decomposition this repository prints is a stack
/// of those.
///
/// A disagreement would mostly not crash. Gray order is not index order, so a
/// greedy fed the survivors as the walk finds them returns a different basis of
/// the same space and every count still matches; an inverse that named a
/// survivor by the wrong index would return a map that is not in the span at
/// all; a budget spent at a different rate would abandon a leaf somewhere else
/// and turn a refutation into an undecided, or the reverse. So the two are run
/// against each other on many spans and compared as maps, and the budget is
/// asserted to stop them at the same element.
///
/// **Over the three widths the repository searches at**: 4x4 from GF(2^4)
/// multiplication, 5x5 from the polynomial fixture, and the 9x9 slices of
/// `<3,3,3>`, whose 261 121-map pool is the largest leaf any published run here
/// reaches. Both storage routes are covered at each, since the table and the
/// on-demand packing are what hands a survivor back once one is found.
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
struct Tally {
    long long compared = 0;
    long long disagreed = 0;
    std::string first;

    void saw(bool agreed, const std::string& what) {
        ++compared;
        if (agreed) return;
        if (disagreed == 0) first = what;
        ++disagreed;
    }

    void report(const std::string& label, long long expected) {
        check::equal(label + ": spans compared", compared, expected);
        check::equal(label + ": and the two scans agreed on every one", disagreed, 0);
        if (disagreed != 0) std::cout << "        first disagreement at " << first << "\n";
    }
};

template <typename Candidates>
void compare(const std::string& what, const Gf2Leaf<Candidates>& leaf, const ReducedBasis& span,
             std::size_t needed, Tally& tally) {
    const std::vector<Matrix> residual = leaf.by_scanning_the_pool(span, needed);
    const std::vector<Matrix> direct = leaf.by_scanning_the_pool_directly(span, needed);
    tally.saw(same(residual, direct), what);
}

/// The spans every shape is asked about: the tensor's own, one slice at a time,
/// and spans built out of pool elements, which have a rank-one basis by
/// construction and so are the only ones where a scan returns anything at all on
/// a product shape.
std::vector<std::pair<std::string, ReducedBasis>> spans_to_ask_about(const Field& field,
                                                                    const RankOnePool& pool,
                                                                    const std::vector<Matrix>& slices,
                                                                    std::size_t width) {
    std::vector<std::pair<std::string, ReducedBasis>> spans;

    ReducedBasis growing(field, width);
    for (std::size_t index = 0; index < slices.size(); ++index) {
        growing.try_add(slices[index]);
        spans.emplace_back("span of " + std::to_string(index + 1) + " slices", growing);
    }

    // Fixed seed: a test that searches a different span each run cannot be
    // reported, and the point here is not to sample the space but to have a
    // dozen spans nobody chose to make either route look right.
    std::mt19937_64 generator(20260820);
    for (std::size_t dimension = 1; dimension <= 12 && dimension <= width; ++dimension) {
        ReducedBasis span(field, width);
        while (span.dimension() < dimension) span.try_add(pool.at(generator() % pool.size()));
        spans.emplace_back("random rank-one span of dimension " + std::to_string(dimension),
                           std::move(span));
    }
    return spans;
}

/// Every target worth asking for, reachable and not.
///
/// `0` never stops the scan, so it walks the whole pool and takes every
/// independent rank-one map in the span. `1` stops at the first survivor, which
/// is the early return inside one left vector's row. The dimension is the leaf's
/// own question, reachable exactly when the span has a rank-one basis. The two
/// above the pool cannot be reached at all, and the largest of them is refused
/// before the first element is examined.
std::vector<std::size_t> targets_for(const ReducedBasis& span, std::size_t width,
                                     std::size_t pool_size) {
    return {0, 1, span.dimension(), width + 1, pool_size + 1};
}

/// A budget stops both routes at the same element.
///
/// Asked with a target of zero, so nothing ends the scan but the pool or the
/// limit: `may_examine` is then called once per index, and the first refusal is
/// at index `limit`. So a limit below the pool size must abandon the leaf and a
/// limit at it must not, on both routes, and the maps collected before the stop
/// must still match.
template <typename Candidates>
void compare_under_a_budget(const std::string& what, const Gf2Leaf<Candidates>& leaf,
                            const ReducedBasis& span, std::size_t pool_size, Tally& tally) {
    for (const std::size_t limit : {std::size_t(1), pool_size / 3, pool_size - 1, pool_size}) {
        SearchBudget for_residual(/*limit=*/1'000'000, /*leaf_limit=*/limit);
        SearchBudget for_direct(/*limit=*/1'000'000, /*leaf_limit=*/limit);
        const std::vector<Matrix> residual = leaf.by_scanning_the_pool(span, 0, &for_residual);
        const std::vector<Matrix> direct =
            leaf.by_scanning_the_pool_directly(span, 0, &for_direct);

        const bool expected = limit < pool_size;
        const bool agreed = same(residual, direct)
                            && for_residual.leaf_abandoned.load() == expected
                            && for_direct.leaf_abandoned.load() == expected
                            && for_residual.tree_fully_walked.load() == for_direct.tree_fully_walked.load();
        tally.saw(agreed, what + " under a leaf limit of " + std::to_string(limit));
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_residual_scan <fixtures>\n";
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

        // With room for the table, which is how every published run here is
        // configured and the route a survivor is handed back through.
        run_limits::set_memory_budget(std::size_t(2) << 30);
        const Gf2Leaf<Addressed> tabled(field, pool, rows, columns);
        check::equal(label + ": the pool is the grid, so the scan carries a residual",
                     tabled.carries_a_residual() ? 1 : 0, 1);

        Tally with_table;
        for (const auto& span : spans) {
            for (const std::size_t needed : targets_for(span.second, width, pool.size())) {
                compare(label + ", " + span.first + ", target " + std::to_string(needed), tabled,
                        span.second, needed, with_table);
            }
        }
        with_table.report(label + ", from the table",
                          static_cast<long long>(spans.size() * 5));

        Tally bounded;
        compare_under_a_budget(label + ", span of every slice", tabled, spans.back().second,
                               pool.size(), bounded);
        bounded.report(label + ", under a budget", 4);

        // And with none, where every element is packed on demand. The same
        // switch the real run makes at a shape whose table would not fit.
        run_limits::set_memory_budget(1);
        const Gf2Leaf<Addressed> onthefly(field, pool, rows, columns);
        run_limits::set_memory_budget(std::size_t(2) << 30);
        check::equal(label + ": and carries one with no table either",
                     onthefly.carries_a_residual() ? 1 : 0, 1);

        Tally without_table;
        for (const auto& span : spans) {
            for (const std::size_t needed : targets_for(span.second, width, pool.size())) {
                compare(label + ", " + span.first + ", target " + std::to_string(needed), onthefly,
                        span.second, needed, without_table);
            }
        }
        without_table.report(label + ", packed on demand",
                             static_cast<long long>(spans.size() * 5));

        // A pool that is not the grid has no residual to carry, and the scan has
        // to notice rather than address a grid that is not there. Materialised,
        // which is the other instantiation and the one every caller without an
        // addressed pool passes.
        std::vector<Matrix> subset;
        for (std::size_t index = 0; index < pool.size() && subset.size() < 60; index += 3) {
            subset.push_back(pool[index]);
        }
        const Gf2Leaf<std::vector<Matrix>> partial(field, subset, rows, columns);
        check::equal(label + ": a pool that is not the grid takes the direct scan",
                     partial.carries_a_residual() ? 1 : 0, 0);

        Tally on_a_subset;
        for (const auto& span : spans) {
            for (const std::size_t needed : targets_for(span.second, width, subset.size())) {
                compare(label + ", " + span.first + ", target " + std::to_string(needed), partial,
                        span.second, needed, on_a_subset);
            }
        }
        on_a_subset.report(label + ", over a pool that is not the grid",
                           static_cast<long long>(spans.size() * 5));
    }

    // The whole materialised grid at the smallest shape, which is the other
    // instantiation over a pool that *is* the grid: the same 225 maps a search
    // over `gf16_multiplication` holds, in the order it holds them.
    {
        const Field field(2);
        const std::vector<Matrix> maps = bilinear_rank::all_rank_one_maps(field, 4, 4);
        const RankOnePool addressed(field, 4, 4);
        const Gf2Leaf<std::vector<Matrix>> materialised(field, maps, 4, 4);
        check::equal("4x4 materialised: the grid is recognised through a vector of maps",
                     materialised.carries_a_residual() ? 1 : 0, 1);

        const auto spans = spans_to_ask_about(field, addressed, {}, 16);
        Tally tally;
        for (const auto& span : spans) {
            for (const std::size_t needed : targets_for(span.second, 16, maps.size())) {
                compare("4x4 materialised, " + span.first + ", target " + std::to_string(needed),
                        materialised, span.second, needed, tally);
            }
        }
        compare_under_a_budget("4x4 materialised, span of dimension 12", materialised,
                               spans.back().second, maps.size(), tally);
        tally.report("4x4 materialised", static_cast<long long>(spans.size() * 5 + 4));
    }

    return check::report("residual scan");
}
