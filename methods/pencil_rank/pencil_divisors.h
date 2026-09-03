#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "prime_power_factors.h"

namespace pencil_rank {

using linear_algebra::ModularMatrix;

/// One elementary divisor of the pencil, and which end of the projective line
/// it sits at.
///
/// A finite one is a power of an irreducible in `x`. An infinite one is a power
/// of the point the pencil reaches when the second slice takes over, and it is
/// always a power of a linear form, so its shape is its exponent alone.
struct ElementaryDivisor {
    PrimePower factor;
    bool at_infinity = false;
};

/// What the diagonal form of the pencil over `GF(p)[x]` says about it.
struct PencilDivisors {
    /// The rank of `first + x second` over the function field `GF(p)(x)`, which
    /// is how many diagonal entries came back nonzero. The pencil is singular
    /// exactly when this is below both dimensions, and that deficiency is the
    /// number of minimal indices on each side.
    std::size_t generic_rank = 0;

    std::vector<ElementaryDivisor> divisors;
};

/// The elementary divisors of the pencil `first + x second` over GF(p).
///
/// Both halves come from a Smith diagonal over `GF(p)[x]`, taken twice.
/// `GF(p)[x]` is Euclidean, which is what makes the diagonalisation
/// terminate on degrees alone; the classical normal form it is the first half
/// of is `[gantmacher1959, Vol. 1, Ch. VI §3, Thm. 3]`, and keys are
/// [`../../references.md`](../../references.md).
///
/// The finite half is the prime powers of the diagonal entries of `first + x
/// second`. The singular blocks of the Kronecker form contribute nothing to it,
/// which is why the regular part is read without ever being isolated. That is
/// `[gantmacher1959, Ch. XII §5]` and not an assumption: an `L_ε` block has no
/// elementary divisors at all, since among its minors of maximal order one
/// equals 1 and one equals `x^ε`, the same holding for its transpose, so the
/// elementary divisors of the whole pencil are those of its regular kernel.
///
/// The infinite half is read from the *reversed* pencil `second + x first`,
/// where the point at infinity has moved to zero. It needs no factorisation at
/// all: the multiplicity of `x` in a polynomial is where its coefficients start
/// being nonzero. That is also why the sides are asymmetric here, and the
/// asymmetry is real rather than an oversight.
///
/// Note that the multiset returned is an invariant of the pencil even though a
/// bare diagonalisation is not unique: the cokernel module decomposes into
/// prime-power cyclic pieces in exactly one way, so the divisibility chain that
/// makes the Smith form itself unique is never needed here and is not computed.
PencilDivisors elementary_divisors(const ModularField& field, const ModularMatrix& first,
                                   const ModularMatrix& second);

/// The invariant factors of a square matrix, as a divisibility chain, each
/// dividing the next.
///
/// These are the diagonal of the Smith form of `x I - square` over `GF(p)[x]`,
/// and unlike the elementary divisors they need the chain that
/// `elementary_divisors` deliberately skips: `[sumi2009, Thm. 3.3]` counts *which
/// invariant polynomials* fail to split, and that count depends on how the prime
/// powers are distributed among them.
///
/// Ascending, so the last entry is Ja'Ja's `p_1`, the minimal polynomial, whose
/// degree is what the cardinality hypothesis is compared against. Entries that
/// are units are dropped, since they say nothing.
std::vector<Polynomial> invariant_factors(const ModularField& field,
                                          const ModularMatrix& square);

}  // namespace pencil_rank
