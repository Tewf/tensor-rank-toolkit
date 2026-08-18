#include "orbit_search.h"

#include <numeric>

#include "rank_one_basis.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

using Permutations = std::vector<std::vector<std::uint32_t>>;
constexpr std::uint32_t kNotHere = static_cast<std::uint32_t>(-1);

/// Where each candidate sits in the list, one buffer per depth so a node and
/// its child never write over each other. Allocated once for the whole search.
using PositionsByDepth = std::vector<std::vector<std::uint32_t>>;

/// One branch per orbit: Covanov's Algorithm 3, lines 6 to 11.
///
/// `candidates` is `H` as pool indices in increasing order, `residual` is `U` as
/// indices into the group, and every element of `residual` stabilises `span` by
/// induction, which is what makes the descent below a single containment test.
bool expand_up_to_impl(const Field& field, ReducedBasis span, const std::vector<Matrix>& pool,
                       const Permutations& action,
                       const std::vector<std::uint32_t>& candidates,
                       const std::vector<std::uint32_t>& residual, std::size_t target,
                       std::size_t depth, SearchBudget& budget, std::vector<Element>& scratch,
                       PositionsByDepth& positions, std::vector<Matrix>& products) {
    if (!budget.try_consume_node()) return false;

    const std::size_t dimension = span.dimension();
    if (dimension > target) return false;
    if (dimension == target) {
        // Against the whole subspace or the whole pool, whichever is smaller,
        // but never against the candidates still standing: a rank-one basis of
        // this subspace may use maps the branch stopped carrying.
        std::vector<Matrix> within =
            rank_one_basis_of(field, span, pool, target, scratch);
        if (within.size() != target) return false;
        products = std::move(within);
        return true;
    }

    std::vector<std::uint32_t>& position = positions[depth];
    for (std::size_t slot = 0; slot < candidates.size(); ++slot) {
        position[candidates[slot]] = static_cast<std::uint32_t>(slot);
    }
    std::vector<char> struck(candidates.size(), 0);

    bool found = false;
    for (std::size_t slot = 0; slot < candidates.size() && !found; ++slot) {
        if (struck[slot]) continue;
        const std::uint32_t chosen = candidates[slot];

        // Everything equivalent to this one is answered by trying this one.
        // Breadth first, because `action` may be a generating set: applying each
        // element once would strike part of the orbit and leave the rest to be
        // searched again, which is sound but wastes exactly what this is for.
        std::vector<std::uint32_t> frontier{chosen};
        while (!frontier.empty()) {
            const std::uint32_t reached = frontier.back();
            frontier.pop_back();
            for (const std::uint32_t element : residual) {
                const std::uint32_t image = action[element][reached];
                const std::uint32_t at = position[image];
                if (at == kNotHere || struck[at]) continue;
                struck[at] = 1;
                frontier.push_back(image);
            }
        }
        struck[slot] = 1;

        if (span.contains(pool[chosen], scratch)) continue;

        ReducedBasis extended = span;
        extended.try_add(pool[chosen]);

        // `residual` already stabilises `span`, so `(span + φ)∘σ = span + φ∘σ`
        // and the whole question is whether that one image landed back inside.
        std::vector<std::uint32_t> narrowed;
        for (const std::uint32_t element : residual) {
            if (extended.contains(pool[action[element][chosen]], scratch)) {
                narrowed.push_back(element);
            }
        }

        // `H'` is the union of the orbits from here on, which is exactly the
        // tail: orbits are struck out in increasing order, so no member of a
        // later orbit sits earlier in the list.
        const std::vector<std::uint32_t> tail(
            candidates.begin() + static_cast<std::ptrdiff_t>(slot), candidates.end());

        if (expand_up_to_impl(field, std::move(extended), pool, action, tail, narrowed, target,
                              depth + 1, budget, scratch, positions, products)) {
            found = true;
        } else if (!budget.exhausted) {
            break;  // gave up rather than ruled out
        }
    }

    for (const std::uint32_t index : candidates) position[index] = kNotHere;
    return found;
}

}  // namespace

bool expand_subspace_up_to_symmetry(const Field& field, const std::vector<Matrix>& subspace,
                           const std::vector<Matrix>& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products) {
    if (subspace.empty()) return false;

    // Filtered, never trusted: an element that does not stabilise the target is
    // the one way this search can report a `NO` that is false.
    const std::vector<Automorphism> stabiliser = stabiliser_of(field, subspace, group);
    const Permutations action = permutation_action_on(field, stabiliser, pool);

    std::vector<std::uint32_t> candidates(pool.size());
    std::iota(candidates.begin(), candidates.end(), std::uint32_t(0));
    std::vector<std::uint32_t> residual(stabiliser.size());
    std::iota(residual.begin(), residual.end(), std::uint32_t(0));

    const ReducedBasis root = linear_algebra::span_of(field, subspace);
    const std::size_t levels = target > root.dimension() ? target - root.dimension() + 1 : 1;
    PositionsByDepth positions(levels, std::vector<std::uint32_t>(pool.size(), kNotHere));

    std::vector<Element> scratch;
    return expand_up_to_impl(field, root, pool, action, candidates, residual, target, 0, budget,
                             scratch, positions, products);
}

}  // namespace bilinear_rank
