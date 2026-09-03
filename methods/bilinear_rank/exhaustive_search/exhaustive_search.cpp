#include "exhaustive_search.h"
#include "rank_one_basis.h"
#include "search_trace.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <stdexcept>

#include "parallel.h"

#include "candidate_pool.h"
#include "span_basis.h"

namespace bilinear_rank {

template <typename Candidates>
std::vector<Matrix> independent_rank_one_maps_in(const Field& field,
                                              const ReducedBasis& reachable,
                                              std::size_t width, const Candidates& pool,
                                              std::size_t needed,
                                              std::vector<Element>& scratch,
                                              SearchBudget* budget) {
    std::vector<Matrix> found;
    ReducedBasis independent(field, width);
    for (std::size_t index = 0; index < pool.size(); ++index) {
        // Once what is left cannot reach the target, the answer is already no.
        if (found.size() + (pool.size() - index) < needed) break;
        // Whereas this break withdraws the answer rather than giving one.
        if (budget != nullptr && !budget->may_examine(index)) break;
        const Matrix& map = pool[index];
        if (!reachable.contains(map, scratch)) continue;
        if (independent.try_add(map)) {
            found.push_back(map);
            if (found.size() == needed) break;
        }
    }
    return found;
}

template std::vector<Matrix> independent_rank_one_maps_in(const Field&, const ReducedBasis&,
                                                      std::size_t, const std::vector<Matrix>&,
                                                      std::size_t, std::vector<Element>&,
                                                      SearchBudget*);
template std::vector<Matrix> independent_rank_one_maps_in(const Field&, const ReducedBasis&,
                                                      std::size_t, const Addressed&, std::size_t,
                                                      std::vector<Element>&, SearchBudget*);

namespace {

/// The search proper, carrying the span down instead of rebuilding it.
///
/// Three things were being redone at every node: the span of the whole subspace,
/// the span again inside the candidate loop, and a full scan of the pool. Only
/// the last is needed, and only at a node that has reached the target
/// dimension. Removing the other two is what makes the depth reachable.
template <typename Candidates>
bool expand_subspace_impl(const Field& field, ReducedBasis span,
                          std::size_t width, const Candidates& pool, std::size_t from,
                          std::size_t target, SearchBudget& budget, std::vector<Element>& scratch,
                          const Gf2Leaf<Candidates>* binary, const std::atomic<bool>* found,
                          std::vector<Matrix>& products, const TraceNode& where = {}) {
    // Somebody else already has a witness, so this subtree cannot change the
    // answer and every node it spends is spent against the shared budget. Without
    // this test the extra subtrees exhausted the budget and turned a proof into an
    // undecided: `matmul_2x2x2 --target 7 --node-limit 20000` gave 7 436 nodes and
    // exit 0 on one thread, and 20 000 nodes and exit 3 on four.
    if (found != nullptr && found->load(std::memory_order_relaxed)) return false;
    if (!budget.try_consume_node()) return false;

    // Opened exactly where the budget counts one, so the trace's node total is
    // the search's and not a second count that could disagree with it. The
    // returns above are not nodes: neither consumed one.
    TraceScope here(where, where.trace != nullptr && where.depth == 0);

    const std::size_t dimension = span.dimension();
    here.dimension(dimension);
    if (dimension > target) {
        here.prune("over-dimension");
        return false;
    }
    if (dimension == target) {
        std::vector<Matrix> within =
            rank_one_basis_of(field, span, pool, target, scratch, &budget, binary);
        if (within.size() != target) {
            here.prune("leaf");
            return false;
        }
        here.adopt(within.size());
        products = std::move(within);  // a rank-one basis of the span: the products
        return true;
    }

    for (std::size_t index = from; index < pool.size(); ++index) {
        const Matrix& map = pool[index];
        if (span.contains(map, scratch)) continue;
        ReducedBasis extended = span;
        extended.try_add(map);
        if (expand_subspace_impl(field, std::move(extended), width, pool, index + 1, target, budget,
                                 scratch, binary, found, products, where.child(here.id(), index))) {
            return true;
        }
        if (!budget.tree_fully_walked) return false;  // gave up rather than ruled out
    }
    return false;
}

}  // namespace

namespace {

/// The public search, written once against either pool. The two overloads below
/// are the whole difference between them.
template <typename Candidates>
bool expand_subspace_over(const Field& field, const std::vector<Matrix>& subspace,
                          const Candidates& pool, std::size_t from, std::size_t target,
                          SearchBudget& budget, std::vector<Matrix>& products,
                          SearchTrace* trace) {
    if (subspace.empty()) return false;
    const std::size_t width = linear_algebra::flattened_width<Field>(subspace);
    const ReducedBasis root = linear_algebra::span_of(field, subspace);

    // The GF(2) case of the leaf test, built once for the whole search and read
    // by every worker. Over any other field it is never built and every leaf
    // takes the path it always took. The pool is packed here rather than by the
    // caller because this is the one scope that outlives no leaf and outlasts
    // every one of them.
    std::optional<Gf2Leaf<Candidates>> binary;
    if (pool.size() != 0 && gf2_leaf_applies(field, pool[0].columns())) {
        binary.emplace(field, pool, pool[0].rows(), pool[0].columns());
    }
    const Gf2Leaf<Candidates>* leaf = binary ? &binary.value() : nullptr;

    if (run_limits::worker_count() <= 1) {
        std::vector<Element> scratch;
        return expand_subspace_impl(field, root, width, pool, from, target, budget, scratch, leaf,
                                    nullptr, products, TraceNode{trace, 0, 0, 0});
    }

    // Above one worker the subtrees interleave and what comes out is not a tree.
    // `decide-rank --trace` refuses the combination before reaching here, so this
    // is the second line of the same defence rather than the only one.
    if (trace != nullptr) {
        throw std::invalid_argument("a trace was asked for on more than one worker");
    }

    // The root node itself, counted here exactly as the recursion counts it, so
    // that a node total does not depend on how many cores answered the question.
    if (!budget.try_consume_node()) return false;

    // One worker per first choice. The subtrees share nothing but the budget,
    // which is atomic, so the only synchronisation left is handing back a
    // witness once somebody has one.
    const std::size_t dimension = root.dimension();
    if (dimension > target) return false;
    if (dimension == target) {
        // A leaf at the root: nothing to spread over cores, so this asks the one
        // leaf test rather than a second copy of one. It used to call the pool
        // scan directly, on the assumption in its own comment that a root leaf is
        // always a scan. It is not: `rank_one_basis_of` picks the subspace walk
        // wherever that is cheaper and the bit-packed route over GF(2), so
        // `--threads 2` answered this leaf by a slower path than `--threads 1`,
        // and by a different one. Threads default to 1, so no published number
        // was ever taken down the wrong branch.
        std::vector<Element> scratch;
        std::vector<Matrix> within = rank_one_basis_of(field, root, pool, target, scratch, &budget, leaf);
        if (within.size() != target) return false;
        products = std::move(within);
        return true;
    }

    std::atomic<bool> found(false);
    std::mutex handover;
    run_limits::parallel_for(pool.size() - from, [&](std::size_t offset) {
        if (found.load(std::memory_order_relaxed)) return;
        if (!budget.tree_fully_walked.load(std::memory_order_relaxed)) return;

        const std::size_t index = from + offset;
        std::vector<Element> scratch;
        if (root.contains(pool[index], scratch)) return;

        ReducedBasis extended = root;
        extended.try_add(pool[index]);

        std::vector<Matrix> mine;
        if (!expand_subspace_impl(field, std::move(extended), width, pool, index + 1, target, budget,
                                  scratch, leaf, &found, mine)) {
            return;
        }
        const std::lock_guard<std::mutex> keep(handover);
        if (!found.exchange(true)) products = std::move(mine);
    });
    return found.load();
}

}  // namespace

bool expand_subspace(const Field& field, const std::vector<Matrix>& subspace,
                     const std::vector<Matrix>& pool, std::size_t from, std::size_t target,
                     SearchBudget& budget, std::vector<Matrix>& products, SearchTrace* trace) {
    return expand_subspace_over(field, subspace, pool, from, target, budget, products, trace);
}

bool expand_subspace(const Field& field, const std::vector<Matrix>& subspace,
                     const RankOnePool& pool, std::size_t from, std::size_t target,
                     SearchBudget& budget, std::vector<Matrix>& products, SearchTrace* trace) {
    return expand_subspace_over(field, subspace, Addressed{pool}, from, target, budget, products,
                                trace);
}

}  // namespace bilinear_rank
