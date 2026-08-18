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
///
/// The finite half is the prime powers of the diagonal entries of `first + x
/// second`. The singular blocks of the Kronecker form contribute nothing to it:
/// each `L` block diagonalises to `[I | 0]` over `GF(p)[x]`, so its invariant
/// factors are all 1 and the regular part is read without ever being isolated.
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

}  // namespace pencil_rank
