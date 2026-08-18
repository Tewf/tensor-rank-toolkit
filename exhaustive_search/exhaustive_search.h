#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

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
/// machine. On a negative answer `exhausted` **true** means "no solution",
/// because the tree really was walked to its end; **false** means "gave up",
/// because the node limit stopped it first. Those are very different claims, and
/// taking the second for the first publishes a lower bound nobody proved.
///
/// The word points the other way in `optimisation/branch_and_bound.cpp`, where
/// `Status::Exhausted` is the budget running out. Same name, opposite sense, and
/// no relation: that one is a different struct in a different namespace.
struct SearchBudget {
    explicit SearchBudget(std::size_t limit = 5'000'000) : node_limit(limit) {}

    std::size_t node_limit;
    std::atomic<std::size_t> nodes_visited{0};
    std::atomic<bool> exhausted{true};  // false once the limit is hit

    /// Atomic because workers share one budget, and written as a compare and
    /// exchange rather than a fetch and add so that a refused node is not
    /// counted: the node totals this repository publishes have to mean the same
    /// thing on one thread and on twelve.
    bool spend() {
        std::size_t seen = nodes_visited.load(std::memory_order_relaxed);
        do {
            if (seen >= node_limit) {
                exhausted.store(false, std::memory_order_relaxed);
                return false;
            }
        } while (!nodes_visited.compare_exchange_weak(seen, seen + 1, std::memory_order_relaxed));
        return true;
    }
};

/// The rank-one maps of `pool` inside a span already built, taken greedily so
/// they stay independent, stopping at `needed` of them.
///
/// This is BDEZ's `HasRankOneBasis` when `needed` is the dimension: a subspace
/// is a solution exactly when this returns that many. Exported so [the
/// quotiented search](../orbit_reduction/orbit_search.h) tests leaves the same way rather than
/// writing a second copy of it.
std::vector<Matrix> independent_rank_one_maps_in(const Field& field, const Span& reachable,
                                                 std::size_t width, const std::vector<Matrix>& pool,
                                                 std::size_t needed, std::vector<Element>& scratch);

/// The rank-one maps of `pool` lying inside the span of `subspace`, taken
/// greedily so they stay independent.
///
/// When there are as many as the subspace has dimensions, they are a rank-one
/// basis of it, and a rank-one basis is an algorithm.
std::vector<Matrix> rank_one_maps_within(const Field& field, const std::vector<Matrix>& subspace,
                                         const std::vector<Matrix>& pool);

/// Extend `subspace` with maps from `pool` (index `from` onward) to dimension
/// exactly `target`, such that the result has a rank-one basis.
///
/// On success `products` holds that basis, which is what the caller needs.
bool expand_subspace(const Field& field, const std::vector<Matrix>& subspace,
                     const std::vector<Matrix>& pool, std::size_t from, std::size_t target,
                     SearchBudget& budget, std::vector<Matrix>& products);

}  // namespace bilinear_rank
