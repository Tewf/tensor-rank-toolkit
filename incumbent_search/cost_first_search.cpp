#include "cost_first_search.h"

#include <algorithm>
#include <set>
#include <utility>

#include "level_lowering_moves.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// The candidate reduced against `V` and scaled to leading entry one.
///
/// `V + <g>` depends on `g` only through this, so two candidates with the same
/// residue are one child and the second is skipped. Measured at the root of
/// `matmul_2x2x2` in
/// [`../exhaustive_search/generating-candidates-from-the-span.md`](../exhaustive_search/generating-candidates-from-the-span.md):
/// 225 candidates, 198 children.
std::vector<Element> residue_of(const Field& field, const ReducedBasis& span, const Matrix& map) {
    std::vector<Element> entries(map.data(), map.data() + map.entry_count());
    span.reduce(entries);
    for (const Element& entry : entries) {
        if (field.isZero(entry)) continue;
        Element scale;
        field.inv(scale, entry);
        for (Element& value : entries) field.mulin(value, scale);
        break;
    }
    return entries;
}

struct Ascent {
    const Field& field;
    const std::vector<Matrix>& pool;
    IncumbentLimits limits;
    IncumbentReport report;
    std::vector<Matrix> best;

    /// The incumbent the bound reads, which is the cheapest cost reached except
    /// where `--below` handed one over that nothing has reached.
    ///
    /// It is held apart from `report.best` so that the report stays a statement
    /// about what was built. Seeding `report.best` with `below + 1` directly
    /// would cut the same branches and then print a cost for an algorithm the
    /// run does not hold.
    std::size_t ceiling = 0;

    /// One child, kept only until its parent has sorted them.
    struct Child {
        std::size_t cost;
        std::vector<Matrix> basis;
    };

    void visit(const std::vector<Matrix>& basis, std::size_t depth) {
        const std::size_t cost = linear_algebra::multiplication_count(field, basis);
        if (cost < report.best) {
            report.best = cost;
            best = basis;
            ++report.improvements;
        }
        ceiling = std::min(ceiling, cost);
        report.deepest = std::max(report.deepest, depth);

        // What `--below` asked for, reached. Nothing under this node can be
        // asked for as well, and nothing above it: the run is over.
        if (limits.below != 0 && cost <= limits.below) {
            report.reached_below = true;
            return;
        }

        // The bound, and the whole of it. Every `W` strictly above this node has
        // `dim W >= basis.size() + 1` and `cost(W) >= dim W`, so nothing below
        // can come in under an incumbent that low.
        if (basis.size() + 1 >= ceiling) {
            ++report.bounded;
            return;
        }
        if (report.nodes >= limits.node_limit) {
            report.exhausted = false;
            return;
        }
        ++report.nodes;

        // The one `p^dim` pass this node pays. It answers both questions asked of
        // `V`: which elements are cheap enough to split, and what every candidate
        // costs once adjoined, since `minimum_weight_basis_with` ranks only the
        // coset the candidate opens.
        const std::vector<std::size_t> known = span_element_ranks(field, basis);
        const std::vector<Matrix> moves =
            limits.whole_pool ? pool
                              : level_lowering_moves(field, basis, known, limits.summand_rank);
        report.moves_offered += moves.size();

        const ReducedBasis span = linear_algebra::span_of(field, basis);
        std::set<std::vector<Element>> reached;
        std::vector<Element> scratch;
        std::vector<Child> children;
        for (const Matrix& move : moves) {
            if (span.contains(move, scratch)) continue;
            if (!reached.insert(residue_of(field, span, move)).second) continue;
            std::vector<Matrix> attempt = minimum_weight_basis_with(field, basis, move, known);
            ++report.children;
            const std::size_t reached_cost = linear_algebra::multiplication_count(field, attempt);
            children.push_back({reached_cost, std::move(attempt)});
        }

        // Cheapest first, ties in generation order, which is fixed. Stable so
        // that the run is reproducible rather than merely deterministic.
        std::stable_sort(children.begin(), children.end(),
                         [](const Child& left, const Child& right) {
                             return left.cost < right.cost;
                         });
        const std::size_t entered =
            limits.width == 0 ? children.size() : std::min(limits.width, children.size());
        for (std::size_t index = 0; index < entered; ++index) {
            visit(children[index].basis, depth + 1);
            if (!report.exhausted || report.reached_below) return;
        }
    }
};

}  // namespace

std::vector<Matrix> search_from_above(const Field& field, const std::vector<Matrix>& start,
                                      const std::vector<Matrix>& pool,
                                      const IncumbentLimits& limits, IncumbentReport* report) {
    // A basis, not whatever the caller happened to hold: the bound reads
    // `basis.size()` as the dimension, and a spanning set with a redundant slice
    // would make it read high and cut branches that were never bounded.
    const std::vector<Matrix> root = minimum_weight_basis(field, start);

    Ascent ascent{field, pool, limits, {}, root, 0};
    ascent.report.best = linear_algebra::multiplication_count(field, root);
    // The incumbent the bound reads, which `--below` may set below anything
    // built. `below + 1` and not `below`, because the question is "at `below` or
    // better" and a subspace of dimension `below` may still cost exactly that.
    ascent.ceiling = limits.below == 0
                         ? ascent.report.best
                         : std::min(ascent.report.best, limits.below + 1);
    ascent.visit(root, 0);

    if (report != nullptr) *report = ascent.report;
    return ascent.best;
}

}  // namespace bilinear_rank
