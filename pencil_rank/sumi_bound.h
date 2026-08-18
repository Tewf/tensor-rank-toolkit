#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"

namespace pencil_rank {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

/// `[sumi2009]`'s count for a regular pencil, and what it proves.
///
/// Let `A` be `n` by `n` and let `k` be the number of invariant polynomials of
/// `A` that do **not** factor into *distinct* linear factors over `K`. Then
///
/// - `[sumi2009, Thm. 3.5]`: `rank_K(E^n; A) >= n + k`, **with no hypothesis**;
/// - `[sumi2009, Thm. 3.3]`, from Ja'Ja': `rank_K(E^n; A) <= n + k`, **provided
///   `Card(K) >= deg p_1(A)`**, where `p_1` is the largest invariant polynomial;
/// - `[sumi2009, Prop. 3.4]`: over `GF(2)` with `A` the companion matrix of
///   `x^3 + x + 1` the rank is at least 5, so that hypothesis cannot be dropped.
///
/// So `n + k` is a proved lower bound over every field, and it is the **rank**
/// whenever the field is large enough, which is a condition a program can check.
/// That is strictly more than
/// [`rank_over_closure`](kronecker_structure.h) gives, which is exact only over
/// the algebraic closure.
///
/// The test for "factors into distinct linear factors over `GF(p)`" is
/// divisibility by `x^p - x`, that polynomial being the product of `(x - a)` over
/// every `a` in the field. Since the invariant polynomials form a divisibility
/// chain, the ones that fail are a prefix, and `k` counts them.
struct SumiBound {
    /// Whether the pencil could be put in the form `(E, A)` at all. False for a
    /// singular pencil, and for a regular one over a field too small to hold an
    /// invertible member: neither theorem is about those and nothing is claimed.
    bool applies = false;

    /// `n + k`. A proved lower bound on the rank whenever `applies`.
    std::size_t bound = 0;

    /// Whether `Card(K) >= deg p_1(A)`, in which case `bound` is the rank.
    bool exact = false;

    /// `k` itself, and the degree the cardinality was compared against, so a
    /// reader can check the arithmetic rather than the code.
    std::size_t failing_factors = 0;
    std::size_t largest_degree = 0;
};

/// Put the pencil in the form `(E, A)` if some member is invertible, then count.
///
/// Any basis of the slice space gives the same tensor, so an invertible member
/// may be taken as the first slice and inverted away: `(M, N)` becomes
/// `(E, M^-1 N)`, which is a change of coordinates and leaves the rank alone.
/// Over a small field there may be no invertible member even when the pencil is
/// regular, since `det` is a form of degree `n` and the projective line has only
/// `p + 1` points; that case reports `applies` false rather than guessing.
SumiBound sumi_bound(const ModularField& field, const std::vector<ModularMatrix>& slices);

}  // namespace pencil_rank
