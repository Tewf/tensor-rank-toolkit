#include "orbit_search.h"

#include "pool_orbits.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <numeric>

#include "parallel.h"
#include "rank_one_basis.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

// Was `std::vector<std::vector<std::uint32_t>>`, one entry per (automorphism,
// pool element). `PoolAction` answers the same question by arithmetic on two
// vector lists, which is 32 768x less at `<4,4,4>` and is what lets this search
// run at all on a shape whose pool is only addressed.
using Permutations = PoolAction;
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
                       PositionsByDepth& positions, const std::atomic<bool>* found_elsewhere,
                       std::vector<Matrix>& products) {
    // Somebody else already has a witness, so this subtree cannot change the
    // answer and every node it spends is spent against the shared budget. The
    // plain search learnt this the expensive way: without the test the extra
    // subtrees exhausted the budget and turned a proof into an undecided.
    // `../exhaustive_search/what-threads-change.md` carries the measurement.
    if (found_elsewhere != nullptr && found_elsewhere->load(std::memory_order_relaxed)) return false;
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
                const std::uint32_t image = action.image(element, reached);
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
            if (extended.contains(pool[action.image(element, chosen)], scratch)) {
                narrowed.push_back(element);
            }
        }

        // `H'` is the union of the orbits from here on, which is exactly the
        // tail: orbits are struck out in increasing order, so no member of a
        // later orbit sits earlier in the list.
        const std::vector<std::uint32_t> tail(
            candidates.begin() + static_cast<std::ptrdiff_t>(slot), candidates.end());

        if (expand_up_to_impl(field, std::move(extended), pool, action, tail, narrowed, target,
                              depth + 1, budget, scratch, positions, found_elsewhere, products)) {
            found = true;
        } else if (!budget.exhausted) {
            break;  // gave up rather than ruled out
        }
    }

    for (const std::uint32_t index : candidates) position[index] = kNotHere;
    return found;
}

/// A candidate list, shared by every branch that starts inside it.
using CandidateList = std::shared_ptr<const std::vector<std::uint32_t>>;

/// A subtree of the quotiented tree, carrying everything it needs to be walked
/// on its own.
///
/// `residual` already stabilises `span` by the induction the algorithm rests on,
/// so a branch handed to a worker is a complete question and not a fragment of
/// one. That is what makes the split safe: the group was filtered once, at entry,
/// and no worker touches the filtering.
///
/// **The candidate list is shared and offset into rather than copied**, because
/// siblings differ only in where their tail starts and a copy each would be
/// `|pool|` words apiece: 1 MB at `⟨3,3,3⟩`, and a node there can have tens of
/// thousands of children. Held this way a branch costs its span and its residual,
/// and the tail is materialised one at a time by whoever walks it.
struct Branch {
    ReducedBasis span;
    CandidateList candidates;
    std::size_t offset = 0;
    std::vector<std::uint32_t> residual;
};

std::vector<std::uint32_t> tail_of(const Branch& branch) {
    return std::vector<std::uint32_t>(
        branch.candidates->begin() + static_cast<std::ptrdiff_t>(branch.offset),
        branch.candidates->end());
}

/// Do one node's work and hand back its children rather than recursing into them.
///
/// This is the body of `expand_up_to_impl` with the recursive call replaced by a
/// push, and it has to stay that way line for line: the node is charged to the
/// budget here and the orbits are struck out here, so a walk split across cores
/// charges and strikes exactly what the sequential one does.
///
/// True when this node was itself the answer, which is the one thing a prefix walk
/// can settle without descending.
bool expand_one(const Field& field, const Branch& node, const std::vector<Matrix>& pool,
                const Permutations& action, std::size_t target, SearchBudget& budget,
                std::vector<Element>& scratch, std::vector<std::uint32_t>& position,
                std::vector<Branch>& children, std::vector<Matrix>& products) {
    if (!budget.try_consume_node()) return false;

    const std::size_t dimension = node.span.dimension();
    if (dimension > target) return false;
    if (dimension == target) {
        std::vector<Matrix> within = rank_one_basis_of(field, node.span, pool, target, scratch);
        if (within.size() != target) return false;
        products = std::move(within);
        return true;
    }

    // Materialised once and then shared by every child, which is what keeps a wide
    // node from costing `|candidates|` copies of `|candidates|`.
    const CandidateList list = std::make_shared<std::vector<std::uint32_t>>(tail_of(node));
    const std::vector<std::uint32_t>& candidates = *list;

    for (std::size_t slot = 0; slot < candidates.size(); ++slot) {
        position[candidates[slot]] = static_cast<std::uint32_t>(slot);
    }
    std::vector<char> struck(candidates.size(), 0);

    for (std::size_t slot = 0; slot < candidates.size(); ++slot) {
        if (struck[slot]) continue;
        const std::uint32_t chosen = candidates[slot];

        std::vector<std::uint32_t> frontier{chosen};
        while (!frontier.empty()) {
            const std::uint32_t reached = frontier.back();
            frontier.pop_back();
            for (const std::uint32_t element : node.residual) {
                const std::uint32_t image = action.image(element, reached);
                const std::uint32_t at = position[image];
                if (at == kNotHere || struck[at]) continue;
                struck[at] = 1;
                frontier.push_back(image);
            }
        }
        struck[slot] = 1;

        if (node.span.contains(pool[chosen], scratch)) continue;

        // Braced rather than default-constructed: `ReducedBasis` holds a field
        // reference and so has no empty state to start from.
        Branch child{node.span, list, slot, {}};
        child.span.try_add(pool[chosen]);
        for (const std::uint32_t element : node.residual) {
            if (child.span.contains(pool[action.image(element, chosen)], scratch)) {
                child.residual.push_back(element);
            }
        }
        children.push_back(std::move(child));
    }

    for (const std::uint32_t index : candidates) position[index] = kNotHere;
    return false;
}

/// The same quotiented walk, its subtrees spread over cores.
///
/// **The quotient removes exactly the parallelism the plain search uses.** That
/// search gives one worker per first choice and there are `|pool|` of them, 225 at
/// `⟨2,2,2⟩`; here the first choices are one per orbit, which is 5, and collapsing
/// them is the whole point. So the split cannot be at the root: the frontier is
/// widened until it is at least as wide as the worker count, and the prefix above
/// it is walked here, sequentially, charged to the budget exactly as the recursion
/// would charge it.
///
/// **What is shared, and what that costs.** The group was filtered by
/// `stabiliser_of` before any of this, and `action` is read-only from then on, so
/// the one way this search can report a false `NO` is untouched by the workers. The
/// budget is shared and atomic, which is what makes a thread count visible: on a
/// refutation every subtree is visited whatever the count, so the node total is
/// exact; on a satisfiable question workers dispatch subtrees the sequential walk
/// never reaches, the total is an upper bound that grows with the workers, and a
/// tight `--node-limit` can turn a proof into an undecided. That is the plain
/// route's finding and it applies here unchanged, which is why
/// `expand_up_to_impl` is given the `found` flag to test before it consumes a node.
///
/// `positions` is per branch rather than shared, because it is one scratch buffer
/// per depth and two workers at one depth would write over each other. It is
/// `levels` rows of `pool.size()`, allocated once per branch: 22 KB at `⟨2,2,3⟩`,
/// against a subtree that is seconds of work.
bool expand_in_parallel(const Field& field, Branch root, const std::vector<Matrix>& pool,
                        const Permutations& action, std::size_t target, SearchBudget& budget,
                        std::vector<Matrix>& products) {
    // Widen the frontier one node at a time, oldest first, until it holds at least
    // as many independent subtrees as there are workers. Oldest first keeps it
    // breadth first, so the prefix stays near the top of the tree where it is a
    // handful of nodes.
    //
    // One node at a time and not one level, so the live frontier is bounded by one
    // node's children plus the worker count rather than by the widest level of the
    // tree.
    std::vector<Branch> frontier;
    frontier.push_back(std::move(root));
    std::size_t taken = 0;

    std::vector<Element> prefix_scratch;
    std::vector<std::uint32_t> position(pool.size(), kNotHere);
    while (taken < frontier.size() && frontier.size() - taken < worker_count()) {
        const Branch node = std::move(frontier[taken]);
        ++taken;
        if (expand_one(field, node, pool, action, target, budget, prefix_scratch, position, frontier,
                       products)) {
            return true;
        }
    }
    if (taken == frontier.size()) return false;  // the prefix was the whole walk

    std::atomic<bool> found(false);
    std::mutex handover;
    parallel_for(frontier.size() - taken, [&](std::size_t offset) {
        if (found.load(std::memory_order_relaxed)) return;
        if (!budget.exhausted.load(std::memory_order_relaxed)) return;

        const Branch& node = frontier[taken + offset];
        const std::size_t reached = node.span.dimension();
        const std::size_t levels = target > reached ? target - reached + 1 : 1;

        std::vector<Element> scratch;
        PositionsByDepth positions(levels, std::vector<std::uint32_t>(pool.size(), kNotHere));
        std::vector<Matrix> mine;
        if (!expand_up_to_impl(field, node.span, pool, action, tail_of(node), node.residual, target,
                               0, budget, scratch, positions, &found, mine)) {
            return;
        }
        const std::lock_guard<std::mutex> keep(handover);
        if (!found.exchange(true)) products = std::move(mine);
    });
    return found.load();
}

}  // namespace

bool expand_subspace_up_to_symmetry(const Field& field, const std::vector<Matrix>& subspace,
                           const std::vector<Matrix>& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products,
                           bool spread_over_cores) {
    if (subspace.empty()) return false;

    // Filtered, never trusted: an element that does not stabilise the target is
    // the one way this search can report a `NO` that is false.
    const std::vector<Automorphism> stabiliser = stabiliser_of(field, subspace, group);
    const Permutations action(field, stabiliser, subspace.front().rows(),
                              subspace.front().columns());

    std::vector<std::uint32_t> candidates(pool.size());
    std::iota(candidates.begin(), candidates.end(), std::uint32_t(0));
    std::vector<std::uint32_t> residual(stabiliser.size());
    std::iota(residual.begin(), residual.end(), std::uint32_t(0));

    const ReducedBasis root = linear_algebra::span_of(field, subspace);
    if (spread_over_cores && worker_count() > 1) {
        Branch whole{root, std::make_shared<std::vector<std::uint32_t>>(candidates), 0, residual};
        return expand_in_parallel(field, std::move(whole), pool, action, target, budget, products);
    }

    const std::size_t levels = target > root.dimension() ? target - root.dimension() + 1 : 1;
    PositionsByDepth positions(levels, std::vector<std::uint32_t>(pool.size(), kNotHere));

    std::vector<Element> scratch;
    return expand_up_to_impl(field, root, pool, action, candidates, residual, target, 0, budget,
                             scratch, positions, nullptr, products);
}

}  // namespace bilinear_rank
