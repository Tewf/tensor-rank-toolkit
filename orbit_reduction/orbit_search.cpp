#include "orbit_search.h"
#include "search_trace.h"

#include <stdexcept>

#include "pool_orbits.h"

#include <atomic>
#include <mutex>
#include <numeric>
#include <optional>

#include "gf2_leaf.h"
#include "isomorph_rejection.h"
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

/// One branch per orbit: Covanov's Algorithm 3, lines 6 to 11.
///
/// `H` is `[from, |pool|)`, `residual` is `U` as indices into the group, and
/// every element of `residual` stabilises `span` by induction, which is what
/// makes the descent below a single containment test.
///
/// **`H` is an index because it has always been a suffix.** A child was handed
/// `candidates[slot..]`, and the root was `0..|pool|`, so the list was
/// `[from, |pool|)` at every node and holding it cost a copy per branch.
template <typename Candidates>
bool expand_up_to_impl(const Field& field, ReducedBasis span, const Candidates& pool,
                       const Permutations& action, std::uint32_t from,
                       const std::vector<std::uint32_t>& residual, std::size_t target,
                       std::size_t depth, SearchBudget& budget, std::vector<Element>& scratch,
                       const std::atomic<bool>* found_elsewhere,
                       const Gf2Leaf<Candidates>* binary, std::vector<Matrix>& products,
                       const TraceNode& where = {}) {
    // Somebody else already has a witness, so this subtree cannot change the
    // answer and every node it spends is spent against the shared budget. The
    // plain search learnt this the expensive way: without the test the extra
    // subtrees exhausted the budget and turned a proof into an undecided.
    // `../exhaustive_search/what-threads-change.md` carries the measurement.
    if (found_elsewhere != nullptr && found_elsewhere->load(std::memory_order_relaxed)) return false;
    if (!budget.try_consume_node()) return false;

    // Opened where the budget counts one, exactly as the plain search does, so
    // the two traces are counting the same thing and the 39.2x between them is
    // the quotient rather than a difference in bookkeeping.
    TraceScope here(where, where.trace != nullptr && depth == 0);

    const std::size_t dimension = span.dimension();
    here.dimension(dimension);
    if (dimension > target) {
        here.prune("over-dimension");
        return false;
    }
    if (dimension == target) {
        // Against the whole subspace or the whole pool, whichever is smaller,
        // but never against the candidates still standing: a rank-one basis of
        // this subspace may use maps the branch stopped carrying.
        std::vector<Matrix> within =
            rank_one_basis_of(field, span, pool, target, scratch, &budget, binary);
        if (within.size() != target) {
            here.prune("leaf");
            return false;
        }
        here.adopt(within.size());
        products = std::move(within);
        return true;
    }

    bool found = false;
    for (std::uint32_t chosen = from; chosen < pool.size() && !found; ++chosen) {
        // Everything equivalent to this one is answered by trying this one, so
        // only the least member of each orbit opens a branch — or, under
        // `--orbit-test generators`, whatever no single element sends earlier,
        // which is a superset of those. `isomorph_rejection.h` derives why the
        // superset changes no verdict.
        if (!opens_a_branch(action, residual, chosen, from)) continue;

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
        // tail: orbits are answered in increasing order, so no member of a later
        // orbit sits earlier. `chosen` and not `chosen + 1`, which is what the
        // list version passed and what keeps the node counts identical; the
        // child skips it on the containment test above.
        if (expand_up_to_impl(field, std::move(extended), pool, action, chosen, narrowed, target,
                              depth + 1, budget, scratch, found_elsewhere, binary,
                              products, where.child(here.id(), chosen))) {
            found = true;
        } else if (!budget.tree_fully_walked) {
            break;  // gave up rather than ruled out
        }
    }

    return found;
}

/// A subtree of the quotiented tree, carrying everything it needs to be walked
/// on its own.
///
/// `residual` already stabilises `span` by the induction the algorithm rests on,
/// so a branch handed to a worker is a complete question and not a fragment of
/// one. That is what makes the split safe: the group was filtered once, at entry,
/// and no worker touches the filtering.
///
/// **The candidate list is an index**, because siblings differ only in where
/// their tail starts and the tail has always been a suffix. A shared list and an
/// offset were a `shared_ptr` and a `size_t` where one `uint32_t` says the same.
struct Branch {
    ReducedBasis span;
    std::uint32_t from = 0;
    std::vector<std::uint32_t> residual;
};

/// Do one node's work and hand back its children rather than recursing into them.
///
/// This is the body of `expand_up_to_impl` with the recursive call replaced by a
/// push, and it has to stay that way line for line: the node is charged to the
/// budget here and the orbits are struck out here, so a walk split across cores
/// charges and strikes exactly what the sequential one does.
///
/// True when this node was itself the answer, which is the one thing a prefix walk
/// can settle without descending.
template <typename Candidates>
bool expand_one(const Field& field, const Branch& node, const Candidates& pool,
                const Permutations& action, std::size_t target, SearchBudget& budget,
                std::vector<Element>& scratch, std::vector<Branch>& children,
                const Gf2Leaf<Candidates>* binary, std::vector<Matrix>& products) {
    if (!budget.try_consume_node()) return false;

    const std::size_t dimension = node.span.dimension();
    if (dimension > target) return false;
    if (dimension == target) {
        std::vector<Matrix> within =
            rank_one_basis_of(field, node.span, pool, target, scratch, &budget, binary);
        if (within.size() != target) return false;
        products = std::move(within);
        return true;
    }

    for (std::uint32_t chosen = node.from; chosen < pool.size(); ++chosen) {
        if (!opens_a_branch(action, node.residual, chosen, node.from)) continue;

        if (node.span.contains(pool[chosen], scratch)) continue;

        // Braced rather than default-constructed: `ReducedBasis` holds a field
        // reference and so has no empty state to start from.
        Branch child{node.span, chosen, {}};
        child.span.try_add(pool[chosen]);
        for (const std::uint32_t element : node.residual) {
            if (child.span.contains(pool[action.image(element, chosen)], scratch)) {
                child.residual.push_back(element);
            }
        }
        children.push_back(std::move(child));
    }

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
template <typename Candidates>
bool expand_in_parallel(const Field& field, Branch root, const Candidates& pool,
                        const Permutations& action, std::size_t target, SearchBudget& budget,
                        const Gf2Leaf<Candidates>* binary, std::vector<Matrix>& products) {
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
    while (taken < frontier.size() && frontier.size() - taken < run_limits::worker_count()) {
        const Branch node = std::move(frontier[taken]);
        ++taken;
        if (expand_one(field, node, pool, action, target, budget, prefix_scratch, frontier,
                       binary, products)) {
            return true;
        }
    }
    if (taken == frontier.size()) return false;  // the prefix was the whole walk

    std::atomic<bool> found(false);
    std::mutex handover;
    run_limits::parallel_for(frontier.size() - taken, [&](std::size_t offset) {
        if (found.load(std::memory_order_relaxed)) return;
        if (!budget.tree_fully_walked.load(std::memory_order_relaxed)) return;

        const Branch& node = frontier[taken + offset];

        std::vector<Element> scratch;
        std::vector<Matrix> mine;
        if (!expand_up_to_impl(field, node.span, pool, action, node.from, node.residual, target,
                               0, budget, scratch, &found, binary, mine)) {
            return;
        }
        const std::lock_guard<std::mutex> keep(handover);
        if (!found.exchange(true)) products = std::move(mine);
    });
    return found.load();
}

template <typename Candidates>
bool expand_up_to_symmetry_over(const Field& field, const std::vector<Matrix>& subspace,
                           const Candidates& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products,
                           bool spread_over_cores, SearchTrace* trace) {
    if (subspace.empty()) return false;

    // Filtered, never trusted: an element that does not stabilise the target is
    // the one way this search can report a `NO` that is false.
    const std::vector<Automorphism> stabiliser = stabiliser_of(field, subspace, group);
    const Permutations action(field, stabiliser, subspace.front().rows(),
                              subspace.front().columns());

    std::vector<std::uint32_t> residual(stabiliser.size());
    std::iota(residual.begin(), residual.end(), std::uint32_t(0));

    // The GF(2) leaf, built once for the whole search, as
    // [`../exhaustive_search/exhaustive_search.cpp`](../exhaustive_search/exhaustive_search.cpp)
    // builds it. Both routes below read `rank_one_basis_of`'s two defaults
    // instead, so every leaf here took the general path and no leaf could be
    // stopped.
    std::optional<Gf2Leaf<Candidates>> packed;
    if (pool.size() != 0) {
        const Matrix first = pool[0];
        if (gf2_leaf_applies(field, first.columns())) {
            packed.emplace(field, pool, first.rows(), first.columns());
        }
    }
    const Gf2Leaf<Candidates>* binary = packed ? &packed.value() : nullptr;

    const ReducedBasis root = linear_algebra::span_of(field, subspace);
    if (spread_over_cores && run_limits::worker_count() > 1) {
        // Above one worker the subtrees interleave and what comes out is not a
        // tree. `decide-rank --trace` refuses the combination before reaching
        // here, so this is the second line of the same defence.
        if (trace != nullptr) {
            throw std::invalid_argument("a trace was asked for on more than one worker");
        }
        Branch whole{root, 0, residual};
        return expand_in_parallel(field, std::move(whole), pool, action, target, budget, binary,
                                  products);
    }

    std::vector<Element> scratch;
    return expand_up_to_impl(field, root, pool, action, 0, residual, target, 0, budget, scratch,
                             nullptr, binary, products, TraceNode{trace, 0, 0, 0});
}

}  // namespace

bool expand_subspace_up_to_symmetry(const Field& field, const std::vector<Matrix>& subspace,
                           const std::vector<Matrix>& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products,
                           bool spread_over_cores, SearchTrace* trace) {
    return expand_up_to_symmetry_over(field, subspace, pool, group, target, budget, products,
                                      spread_over_cores, trace);
}

bool expand_subspace_up_to_symmetry(const Field& field, const std::vector<Matrix>& subspace,
                           const RankOnePool& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products,
                           bool spread_over_cores, SearchTrace* trace) {
    return expand_up_to_symmetry_over(field, subspace, Addressed{pool}, group, target, budget,
                                      products, spread_over_cores, trace);
}

}  // namespace bilinear_rank
