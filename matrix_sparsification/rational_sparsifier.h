#pragma once

#include "types.h"

/// The sparsest basis over `Q`, exactly, by the matroid greedy.
///
/// `nnz(U V)` over invertible `V` is the total weight of a basis of `U`'s column
/// space, and every basis arises from some `V`, so the problem *is* "choose a
/// basis of this space of least total weight". Linear independence is a matroid
/// `[oxley, Prop. 1.1.1]` and the greedy returns a minimum-weight basis of any
/// matroid under any weight `[oxley, Lem. 1.8.3]`, so scanning the space in
/// ascending weight and keeping whatever is not already spanned is the optimum
/// and not a heuristic. **This is the same theorem
/// [`finite_field_sparsifier.h`](finite_field_sparsifier.h) rests on, over a
/// field where the space cannot be enumerated.**
///
/// **The composition is `[beniamini2020]`'s own and is already proved there.**
/// Algorithm 2, which they take from `[gottlieb2010]`, is exactly this greedy:
/// start from the empty set and repeatedly call an oracle for Sparsest
/// Independent Vector (their Problem 2.15, "a vector in the row space of `U`,
/// not in the span of the rows already settled, with a minimal number of
/// nonzero entries"), and they state that with a true SIV oracle it returns an
/// exact solution. Nothing here is a new composition; what is new is doing the
/// oracle exactly and paying for it in seconds.
///
/// **Why the scan can be by support and still be complete.** A codeword of
/// weight `w` is supported on some `w` columns, so walking column subsets in
/// ascending size walks the space in ascending weight. What makes it finish is
/// that the subsets stay small: pick any information set, and the systematic
/// basis it gives has every vector of weight at most `n - r + 1`, so at every
/// step of the greedy at least one of those `r` vectors is outside the span of
/// the fewer than `r` already held. **No greedy weight ever exceeds `n - r + 1`**,
/// which is the same ceiling the Singleton bound puts on the minimum distance,
/// and the scan can stop there. Measured on the three operators of the rank-23
/// `⟨3,3,3⟩` scheme `Grey-221`, where `n` is 23 and `r` is 9: the largest weight
/// the greedy ever took was 6, 6 and 7, against a ceiling of 15.
///
/// **This retires the widening that looked necessary.** Reading the enumeration
/// as "column subsets of size `r - 1`" invites the worry that once some vectors
/// are settled, the lightest vector outside their span has fewer than `r - 1`
/// zeros and is invisible. The bound above says it cannot: every vector the
/// greedy takes has at least `r - 1` zeros, so the floor
/// [`oracle_sparsifier.h`](oracle_sparsifier.h)'s top-down walk stops at is
/// exactly the right one, and searching below it would only cost time.
namespace matrix_sparsification {

/// A minimum-weight basis of the row space of `rows`, as a matrix of the same
/// shape.
///
/// Scans column supports in ascending size and keeps every codeword supported
/// there that raises the rank, which is the greedy with the settled set built up
/// from empty. A rank-deficient operator comes back with the trailing rows zero:
/// a basis is what the question asks for and it has fewer vectors than the
/// operator has rows.
Matrix sparsest_basis_over_the_rationals(const Field& field, const Matrix& rows);

}  // namespace matrix_sparsification
