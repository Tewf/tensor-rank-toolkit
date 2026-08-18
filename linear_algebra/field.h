#pragma once

#include <givaro/modular.h>
#include <givaro/qfield.h>

#include <cstdint>

#include "matrix.h"

/// Exact arithmetic, from Givaro. Both strands of this repository are searches
/// over ranks and over counts of nonzero entries, so an answer that is nearly
/// right is not a slightly worse answer, it is an answer to a different
/// question. Nothing here is ever a float.
namespace linear_algebra {

/// The finite-field side: the rank of a bilinear map over GF(p). The primes in
/// play are tiny, so a machine integer representation is ample.
using ModularField = Givaro::Modular<int64_t>;

/// Whether arithmetic modulo `characteristic` is a field at all.
///
/// Every rank claim here is a claim about GF(p). Modulo a composite there are
/// zero divisors, so a "rank" over it is not the quantity any of this measures
/// and no backend models it: both CNF encoders refuse, and cvc5's `QF_FF`
/// decides *prime* fields. Extension fields are reached the other way, as the
/// `gf4`, `gf8` and `gf16` fixtures do it: GF(2^k) multiplication is a bilinear
/// map over GF(2), so the field stays prime and the tensor grows.
///
/// Trial division, because the primes in play are single digits.
inline bool is_prime(int64_t characteristic) {
    if (characteristic < 2) return false;
    for (int64_t divisor = 2; divisor * divisor <= characteristic; ++divisor) {
        if (characteristic % divisor == 0) return false;
    }
    return true;
}

/// The rational side: sparsifying the operators of a fast multiplication
/// algorithm, whose entries are fractions like 4/9. This implementation carries
/// them exactly as rationals, enabling precise zero counting.
using RationalField = Givaro::QField<Givaro::Rational>;

template <class Field>
using MatrixOver = Matrix<typename Field::Element>;

using ModularMatrix = MatrixOver<ModularField>;
using RationalMatrix = MatrixOver<RationalField>;

}  // namespace linear_algebra
