#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

/// The rank-one maps worth adjoining to a subspace, generated from the subspace
/// rather than scanned out of the pool.
///
/// **Which adjunctions can lower the cost, exactly.** Write `R[r]` for the span
/// of the elements of `V` of rank at most `r`, as
/// [`../descent_search/sorted_span.h`](../descent_search/sorted_span.h) does, and
/// `W = V + <g>` for a rank-one `g` outside `V`. Setting
/// `e_r = dim R[r](W) - dim R[r](V)`, Abel summation on the cost identity gives
///
///     cost(W) - cost(V) = maxrank - sum_{r < maxrank} e_r
///
/// and generically every `e_r` is 1, since `R[r](W) = R[r](V) + <g>`, so a generic
/// adjunction costs exactly one more multiplication. **The cost can only fail to
/// rise when some `e_r` is 2 or more**, which needs an element `v + λg` of rank at
/// most `r` whose `v` is outside `R[r](V)`. A rank-one `g` moves any rank by at
/// most one, so that element has `rank(v + λg) = rank(v) - 1`.
///
/// So every adjunction that does not simply cost one more is a **level-lowering
/// summand** of some element of `V`: `g` with `rank(v - g) = rank(v) - 1`. That is
/// what this file generates, and it is generated in closed form rather than found
/// by scanning: for `v = C R` a rank factorisation with `C` of full column rank
/// and `R` of full row rank, the level-lowering summands are exactly
///
///     g = (C a)(bᵀ R)   for a ≠ 0 and bᵀa = 1,
///
/// because `v - g = C(I - a bᵀ)R` and `I_r - a bᵀ` drops to rank `r - 1` exactly
/// when `bᵀa = 1`. There are `(p^r - 1) p^(r-1) / (p - 1)` of them, so an element
/// of rank 3 over GF(2) offers **28** candidates where the pool of 7x7 rank-one
/// maps offers 16 129.
///
/// **What that does not do is remove the pool.** The count above is 8128 at rank
/// 7, so the saving is in taking the *low-rank* elements of `V` and nothing else,
/// which is a restriction and not an equivalence:
/// [`../exhaustive_search/generating-candidates-from-the-span.md`](../exhaustive_search/generating-candidates-from-the-span.md)
/// settles the same question for the deficit and reaches the same wall.
namespace bilinear_rank {

/// Every rank-one `g` with `rank(matrix - g) == rank(matrix) - 1`, one per map
/// and not one per `(a, b)` pair.
///
/// Empty when `matrix` is zero, and the whole scalar class of `matrix` itself
/// when it has rank one.
std::vector<Matrix> level_lowering_summands(const Field& field, const Matrix& matrix);

/// The same over a span: the union over the elements of `span(slices)` whose rank
/// is between 2 and `cutoff`, deduplicated.
///
/// `element_ranks` is what
/// [`span_element_ranks`](../descent_search/minimum_weight_basis.h) returns for
/// the same slices, so the caller's own filtration pass is not repeated here.
std::vector<Matrix> level_lowering_moves(const Field& field, const std::vector<Matrix>& slices,
                                         const std::vector<std::size_t>& element_ranks,
                                         std::size_t cutoff);

}  // namespace bilinear_rank
