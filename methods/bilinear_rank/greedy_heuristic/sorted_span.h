#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

/// A span held as its rank filtration, so the two questions the search asks of
/// one are answers rather than computations.
///
/// Write `R[r]` for the span of the elements of rank at most `r`. Then
/// `0 = R[0] ⊆ R[1] ⊆ … ⊆ R[maxrank] = V`, and both of these fall out of the
/// dimensions alone:
///
///     V has a rank-one basis   <=>   dim R[1] == dim V
///     cost(V)                   =    sum_r  r * (dim R[r] - dim R[r-1])
///
/// **The cost identity is the matroid greedy, rearranged.** The greedy takes
/// elements in ascending rank, so once it has seen every element of rank at most
/// `r` it holds a maximal independent subset of them, which is a basis of `R[r]`
/// of size `dim R[r]`. It therefore accepted exactly `dim R[r] - dim R[r-1]`
/// elements of rank exactly `r`. Rado-Edmonds makes that basis minimum-weight,
/// and `[nakatsukasa2017, Thm. 2.1]` states the same exactness for this problem;
/// keys are [`../../../references.md`](../../../references.md). So this is not an
/// approximation of `minimum_weight_basis` but the same number by another route,
/// which [`tests/test_sorted_span.cpp`](tests/test_sorted_span.cpp) asserts on
/// every fixture rather than leaving to the argument above.
///
/// **What it is for.** `multiplication_count` answers `cost` by building a
/// minimum-weight basis, which sorts `p^dim` elements to take at most `dim` of
/// them. Here the sort is a counting pass over at most `min(n,m) <= 16` buckets,
/// nothing is stored but one byte per element while the pass runs, and the state
/// that survives is `16` reduced bases: **24 KB at `⟨4,4,4⟩` against the
/// `p^dim * 16` bytes `require_room` refuses.**
///
/// **What it is not for.** Building it still visits `p^dim` elements, so it turns
/// a memory refusal into a time cost and not into an answer. Whether that is the
/// cheaper side is the crossover
/// [`../exhaustive/rank_one_basis.h`](../exhaustive/rank_one_basis.h)
/// already decides per call: `p^dim` against `|pool|`.
///
/// **It is not the exact search's leaf**, which is the reading `has_rank_one_basis`
/// invites. That leaf's walk route visits the same elements, stops at `dim V` of
/// them, and asks a rank-one test where this asks a Gaussian rank; and at a *node*
/// the cost identity was measured not to fire. Both verdicts, and the one place
/// this does belong: [`../../../writeup/how-the-search-works/what-to-wire.md`](../../../writeup/how-the-search-works/what-to-wire.md).
namespace bilinear_rank {

class SortedSpan {
   public:
    /// `ranks_without_last` is the same optimisation `minimum_weight_basis`
    /// takes and means the same thing: the ranks of the span of every slice but
    /// the last, in enumeration order. Passing it is what makes the two routes
    /// comparable at all, since half the enumeration is a rank neither of them
    /// then computes.
    SortedSpan(const Field& field, const std::vector<Matrix>& slices,
               const std::vector<std::size_t>& ranks_without_last = {});

    std::size_t dimension() const { return dimension_; }

    /// The number of multiplications a basis of this span costs: the least
    /// `sum of ranks` over its bases.
    ///
    /// **It is not the fewest rank-one maps whose span contains this one**, which
    /// it said until 2026-08-20, and the difference is the whole soundness of a
    /// pruning rule somebody will propose. It is an *upper* bound on that number
    /// and can be strictly larger: over GF(2), `V = span(diag(1,1,0),
    /// diag(0,1,1))` has every nonzero element of rank 2, so `cost(V) = 4`, while
    /// `V` sits inside `span(E11, E22, E33)`, three rank-one maps.
    ///
    /// So `cost(V) <= k` **finds** a `k`-product algorithm and `cost(V) > k`
    /// **refutes nothing**: the example above is a live solution at `k = 3` that
    /// such a test would throw away.
    ///
    /// **It is not monotone either, and the same example shows it.** Adjoining
    /// the rank-one `diag(1,0,0)` to that plane gives `span(E11, E22, E33)`:
    /// `cost` falls from 4 to 3 while the dimension rises from 2 to 3. So a
    /// search *can* reach its target's cost before it reaches its target's
    /// dimension, which is the one way this could shorten a branch. What it was
    /// measured never to do is fall far enough, soon enough:
    /// [`../exhaustive/what-a-node-cannot-tell-you.md`](../exhaustive/what-a-node-cannot-tell-you.md).
    std::size_t cost() const;

    /// Whether the span is covered by its own rank-one elements, which is the
    /// question every leaf of the exact search asks.
    bool has_rank_one_basis() const { return reached_[0] == dimension_; }

    /// `dim R[r]`, for `r` from 1. Out of range reads as the whole span, since
    /// no element has rank above `maxrank`.
    std::size_t reached_by_rank(std::size_t rank) const;

   private:
    std::size_t dimension_ = 0;
    /// `reached_[r - 1] = dim R[r]`, one entry per attainable rank.
    std::vector<std::size_t> reached_;
};

}  // namespace bilinear_rank
