#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "candidate_pool.h"
#include "search_trace.h"

/// The exact search: not "can this be improved?" but "is there one of size k?".
///
/// This is an implementation of a pre-existing published algorithm, complete
/// and exponential for the question it asks, in contrast to the
/// [heuristic descent methods](../descent_search/minimise_rank.h) which offer
/// no optimality guarantee.
///
/// **What it decides, precisely.** Given a subspace `W` already containing the
/// map, it decides whether `W` can be extended to a space of dimension `k` that
/// has a basis made entirely of rank-one maps, which is a `k`-multiplication
/// algorithm. Sweeping `k` upward gives the fewest products *among
/// decompositions containing `W`*. Starting from `W = {}` would give the true
/// minimum, and costs `C(|pool|, k)`: around 10^30 for the fixtures here, so it
/// is offered and guarded rather than promised.
namespace bilinear_rank {

/// How much of the tree a search was allowed, and how much it used.
///
/// Without this an infeasible question is indistinguishable from a slow
/// machine. On a negative answer `tree_fully_walked` **true** means "no
/// solution", because the tree really was walked to its end; **false** means
/// "gave up", because the node limit stopped it first. Those are very different
/// claims, and taking the second for the first publishes a lower bound nobody
/// proved. The name says which side of `integer_programme`'s `Status::Exhausted`
/// it is not: that one is the budget running out, this one is the walk
/// finishing. (The field was `exhausted` until the two words collided.)
struct SearchBudget {
    explicit SearchBudget(std::size_t limit = 5'000'000,
                          std::size_t leaf_limit = 100'000'000)
        : node_limit(limit), leaf_element_limit(leaf_limit) {}

    std::size_t node_limit;
    std::atomic<std::size_t> nodes_visited{0};
    std::atomic<bool> tree_fully_walked{true};  // false once either limit is hit

    /// What one leaf may examine before the search abandons it.
    ///
    /// The node limit bounds how many leaves are reached and bounds nothing
    /// inside one, and a leaf is a whole scan of the pool or a whole walk of the
    /// subspace. That was invisible while the pool had to be materialised,
    /// because a pool too large to scan was refused by `require_room` before any
    /// leaf saw it. An addressed pool removed the refusal, and with it the only
    /// thing that was bounding the leaf: at `<4,4,4>` a single leaf is
    /// 4 294 836 225 maps formed one at a time, **measured at 129.1 ns each, so
    /// about 9 minutes**, and `--node-limit 1` could not make the run return.
    /// It was 785 ns and 0.9 hours before the element was formed in words.
    ///
    /// PROVISIONAL at 100 000 000, which is 78 s of that scan and 7.8 s of a
    /// subspace walk (measured at 78 ns an element on `<4,4,4>` at dimension
    /// 27). It is 383x the largest leaf any published run here reaches, the
    /// 261 121-map pool of `<3,3,3>`, so no measurement in this repository moves.
    /// `--leaf-limit` changes it per run.
    std::size_t leaf_element_limit;

    /// Which of the two limits withdrew the answer, for the report only. Both
    /// clear `tree_fully_walked` and a run that gives up needs to say which to raise,
    /// because raising the other one changes nothing and looks like the tool
    /// ignoring the flag.
    std::atomic<bool> leaf_abandoned{false};

    /// Atomic because workers share one budget, and written as a compare and
    /// exchange rather than a fetch and add so that a refused node is not
    /// counted.
    ///
    /// **Sharing one budget is what makes a thread count visible in an answer.**
    /// A refutation spends the same nodes whoever spends them, so its total is
    /// exact at any thread count. A witness stops the search early, and the
    /// workers that were already running spend against this same counter, so the
    /// total is an upper bound and a tight limit can be exhausted before the
    /// winner reports. That is measured in `MEASURING.md`, and it is why
    /// `expand_subspace_impl` tests `found` before consuming a node.
    bool try_consume_node() {
        std::size_t seen = nodes_visited.load(std::memory_order_relaxed);
        do {
            if (seen >= node_limit) {
                tree_fully_walked.store(false, std::memory_order_relaxed);
                return false;
            }
        } while (!nodes_visited.compare_exchange_weak(seen, seen + 1, std::memory_order_relaxed));
        return true;
    }

    /// Whether a leaf that has already examined `examined` may examine another.
    ///
    /// Marks the answer inconclusive when it may not, which is the whole of what
    /// makes this sound: an abandoned leaf hands back fewer maps than the target
    /// and so reads to its caller as "no rank-one basis here", which would be a
    /// refutation nobody proved. `tree_fully_walked` false turns the run's verdict into
    /// GAVE UP, so the only thing an abandoned leaf can do is withdraw a NO.
    bool may_examine(std::size_t examined) {
        if (examined < leaf_element_limit) return true;
        leaf_abandoned.store(true, std::memory_order_relaxed);
        tree_fully_walked.store(false, std::memory_order_relaxed);
        return false;
    }
};

/// The rank-one maps of `pool` inside a span already built, taken greedily so
/// they stay independent, stopping at `needed` of them.
///
/// This is BDEZ's `HasRankOneBasis` when `needed` is the dimension: a subspace
/// is a solution exactly when this returns that many. Exported so [the
/// quotiented search](../orbit_reduction/orbit_search.h) tests leaves the same way rather than
/// writing a second copy of it.
/// Templated on where the candidates come from. A `std::vector<Matrix>` already
/// satisfies the two methods this needs, `size()` and `operator[]`, so every
/// existing caller passes one unchanged; `Addressed` in
/// [`candidate_pool.h`](../descent_search/candidate_pool.h) satisfies them by
/// building each map on demand. Instantiated for exactly those two in the source.
///
/// `budget` bounds the scan, and `nullptr` leaves it unbounded for the callers
/// that have no budget to give: a pool small enough to hold is small enough to
/// scan, so the bound matters only where the pool is addressed.
template <typename Candidates>
std::vector<Matrix> independent_rank_one_maps_in(const Field& field, const ReducedBasis& reachable,
                                                 std::size_t width, const Candidates& pool,
                                                 std::size_t needed, std::vector<Element>& scratch,
                                                 SearchBudget* budget = nullptr);

/// Extend `subspace` with maps from `pool` (index `from` onward) to dimension
/// exactly `target`, such that the result has a rank-one basis.
///
/// On success `products` holds that basis, which is what the caller needs.
bool expand_subspace(const Field& field, const std::vector<Matrix>& subspace,
                     const std::vector<Matrix>& pool, std::size_t from, std::size_t target,
                     SearchBudget& budget, std::vector<Matrix>& products,
                     SearchTrace* trace = nullptr);

/// The same search over an **addressed** pool, which is never materialised.
///
/// Same nodes, same answer, and `O(p^rows + p^columns)` of memory instead of
/// `O(p^rows * p^columns)` matrices. This is the odometer of `[yang2025]`: the
/// pool is walked by index and each map is built when the index is reached.
///
/// **What it costs is a rebuild per visit rather than per run.** The recursion
/// carries an index down and resumes from it, so a map at a given index is built
/// once per node that reaches it, not once. That is the trade, and it is the
/// right one exactly where the materialised pool cannot be had at all: at
/// `⟨4,4,4⟩` it is 4.3e9 maps and 8.2 TiB, which `require_room` refuses in
/// milliseconds, so the choice there is between a slower search and no search.
///
/// `decide-rank` picks this automatically when the materialised pool would be
/// refused, and says which it used.
bool expand_subspace(const Field& field, const std::vector<Matrix>& subspace,
                     const RankOnePool& pool, std::size_t from, std::size_t target,
                     SearchBudget& budget, std::vector<Matrix>& products,
                     SearchTrace* trace = nullptr);

}  // namespace bilinear_rank
