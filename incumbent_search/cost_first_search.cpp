#include "cost_first_search.h"

#include <algorithm>
#include <set>
#include <utility>

#include "level_lowering_moves.h"
#include "minimum_weight_basis.h"
#include "orbit_moves.h"
#include "parallel.h"
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
    const std::vector<Automorphism>& ambient;
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

    /// `cost` is the subspace's own cost, carried in rather than recomputed: the
    /// greedy that built this basis summed the ranks it picked, and both callers
    /// below already hold that sum.
    void visit(const std::vector<Matrix>& basis, std::size_t cost, std::size_t depth) {
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
        const std::vector<Matrix> offered =
            limits.whole_pool ? pool
                              : level_lowering_moves(field, basis, known, limits.summand_rank);
        report.moves_offered += offered.size();

        // One per orbit of this node's own stabiliser, where a group was given
        // and the run asked for it. `moves_up_to_symmetry` hands back `offered`
        // itself when there is nothing to quotient by, so the unasked run costs
        // nothing here and enters exactly the moves it always did.
        std::size_t stabiliser = 0;
        const std::vector<Matrix> moves =
            limits.quotient_moves
                ? moves_up_to_symmetry(field, basis, ambient, offered, &stabiliser)
                : offered;
        report.moves_entered += moves.size();
        if (limits.quotient_moves && !ambient.empty()) {
            report.smallest_stabiliser = report.largest_stabiliser == 0
                                             ? stabiliser
                                             : std::min(report.smallest_stabiliser, stabiliser);
            report.largest_stabiliser = std::max(report.largest_stabiliser, stabiliser);
        }

        // Which moves are children, then what each child costs, in that order and
        // not interleaved — because only the second half may leave this core.
        //
        // **The filter carries state and stays sequential.** `reached` decides a
        // move by what earlier moves put in it, so which of two moves with the
        // same residue survives is the offering order and nothing else. Running
        // it on one core keeps that order, keeps `report.children`, and keeps the
        // slot each survivor lands in.
        //
        // **The basis each survivor opens carries none and goes to the workers.**
        // `minimum_weight_basis_with` reads `basis`, `known` and its own move,
        // none of which anybody writes, and each answer is written to its own
        // slot. So the children are the same children in the same order at any
        // `--threads`, and the `stable_sort` below sees the same vector — which
        // is what makes this the one search here that threads without an argument
        // about node counts. It is the same trade
        // [`../descent_search/minimise_rank.cpp`](../descent_search/minimise_rank.cpp)
        // already makes with the identical call.
        const ReducedBasis span = linear_algebra::span_of(field, basis);
        std::set<std::vector<Element>> reached;
        std::vector<Element> scratch;

        std::vector<const Matrix*> surviving;
        for (const Matrix& move : moves) {
            if (span.contains(move, scratch)) continue;
            if (!reached.insert(residue_of(field, span, move)).second) continue;
            surviving.push_back(&move);
        }
        report.children += surviving.size();

        std::vector<Child> children(surviving.size());
        parallel_for(surviving.size(), [&](std::size_t slot) {
            // The cost comes back with the basis. It is the sum of the ranks the
            // greedy picked, which it knew before it had a basis to hand over;
            // ranking those matrices again here was one Gaussian elimination per
            // basis element per child, 17 371 of them on `cyclic_f2_7` and
            // 1 258 756 on the `gf32_multiplication` run that reaches 13.
            std::size_t cost = 0;
            std::vector<Matrix> attempt =
                minimum_weight_basis_with(field, basis, *surviving[slot], known, &cost);
            children[slot].cost = cost;
            children[slot].basis = std::move(attempt);
        });

        // Cheapest first, ties in generation order, which is fixed. Stable so
        // that the run is reproducible rather than merely deterministic.
        std::stable_sort(children.begin(), children.end(),
                         [](const Child& left, const Child& right) {
                             return left.cost < right.cost;
                         });
        const std::size_t entered =
            limits.width == 0 ? children.size() : std::min(limits.width, children.size());
        for (std::size_t index = 0; index < entered; ++index) {
            visit(children[index].basis, children[index].cost, depth + 1);
            if (!report.exhausted || report.reached_below) return;
        }
    }
};

}  // namespace

std::vector<Matrix> search_from_above(const Field& field, const std::vector<Matrix>& start,
                                      const std::vector<Matrix>& pool,
                                      const IncumbentLimits& limits, IncumbentReport* report,
                                      const std::vector<Automorphism>& ambient) {
    // A basis, not whatever the caller happened to hold: the bound reads
    // `basis.size()` as the dimension, and a spanning set with a redundant slice
    // would make it read high and cut branches that were never bounded.
    std::size_t root_cost = 0;
    const std::vector<Matrix> root = minimum_weight_basis(field, start, {}, &root_cost);

    Ascent ascent{field, pool, ambient, limits, {}, root, 0};
    ascent.report.best = root_cost;
    // The incumbent the bound reads, which `--below` may set below anything
    // built. `below + 1` and not `below`, because the question is "at `below` or
    // better" and a subspace of dimension `below` may still cost exactly that.
    ascent.ceiling = limits.below == 0
                         ? ascent.report.best
                         : std::min(ascent.report.best, limits.below + 1);
    ascent.visit(root, root_cost, 0);

    if (report != nullptr) *report = ascent.report;
    return ascent.best;
}

}  // namespace bilinear_rank
