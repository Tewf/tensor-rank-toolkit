#include "cost_first_search.h"

#include <algorithm>
#include <cmath>
#include <limits>
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

/// The least a strict descendant can cost, given the two inequalities.
///
/// `s == 0` disables the second one and returns `a + 1`, which is the bound this
/// search has always used, so the general form has the old one as a special case
/// and switching it off restores every published count exactly.
std::size_t least_descendant_cost(std::size_t a, std::size_t c, std::size_t s) {
    if (s == 0) return a + 1;
    const double crossing = c > a ? static_cast<double>(c - a) / static_cast<double>(1 + s) : 0.0;
    std::size_t least = std::numeric_limits<std::size_t>::max();
    for (const double where : {std::floor(crossing), std::ceil(crossing)}) {
        const std::size_t steps = where < 1.0 ? 1 : static_cast<std::size_t>(where);
        const std::size_t rises = a + steps;
        const std::size_t falls = c > s * steps ? c - s * steps : 0;
        least = std::min(least, std::max(rises, falls));
    }
    return least;
}

struct Ascent {
    const Field& field;
    const std::vector<Matrix>& pool;
    const std::vector<Automorphism>& ambient;
    /// Null unless a caller asked what the tree repeats. Nothing reads it but
    /// the two `record` calls below, and neither changes what is entered.
    SpanCensus* census;
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
        // Before the bound and before the budget, because a node the tree
        // entered is a node the tree entered whichever of the two turns it back.
        if (census != nullptr) census->entered.record(field, basis);

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
        if (least_descendant_cost(basis.size(), cost, limits.cost_drop_bound) >= ceiling) {
            ++report.bounded;
            return;
        }
        if (report.nodes >= limits.node_limit) {
            report.exhausted = false;
            return;
        }
        ++report.nodes;
        // The same span again, now that it is known to be one the tree pays a
        // subtree for rather than one the bound turns back on arrival.
        if (census != nullptr) census->expanded.record(field, basis);

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

        // Every child, entered or not, because the costing above is what a
        // repeated child costs and it is already paid by the time the beam picks.
        if (census != nullptr) {
            for (const Child& child : children) census->children.record(field, child.basis);
        }

        // What one move is worth, over every child and not only the entered
        // ones: the bound a future version wants needs the largest drop any
        // single step can produce, and the beam would only ever see the cheapest
        // few. Sequential, after the workers are done, because it writes to the
        // report.
        for (const Child& child : children) {
            if (cost <= child.cost) continue;
            const std::size_t drop = cost - child.cost;
            report.largest_drop = std::max(report.largest_drop, drop);
            // The bound's own assumption, checked on every child it was used to
            // cut past. A violation means a branch was cut that could have held
            // the answer, which the command turns into a refusal rather than a
            // quieter number.
            if (limits.cost_drop_bound != 0 && drop > limits.cost_drop_bound) ++report.drops_exceeded;
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
            visit(children[index].basis, children[index].cost, depth + 1);
            if (!report.exhausted || report.reached_below) return;
        }
    }
};

}  // namespace

std::vector<Matrix> search_from_above(const Field& field, const std::vector<Matrix>& start,
                                      const std::vector<Matrix>& pool,
                                      const IncumbentLimits& limits, IncumbentReport* report,
                                      const std::vector<Automorphism>& ambient,
                                      SpanCensus* census) {
    // A basis, not whatever the caller happened to hold: the bound reads
    // `basis.size()` as the dimension, and a spanning set with a redundant slice
    // would make it read high and cut branches that were never bounded.
    std::size_t root_cost = 0;
    const std::vector<Matrix> root = minimum_weight_basis(field, start, {}, &root_cost);

    Ascent ascent{field, pool, ambient, census, limits, {}, root, 0};
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
