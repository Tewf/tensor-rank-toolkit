#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "field.h"

namespace pencil_rank {

using linear_algebra::ModularField;

/// A univariate polynomial over GF(p): coefficients ascending by degree, with
/// no trailing zero, so the zero polynomial is the empty vector.
///
/// Givaro ships `Poly1FactorDom`, which would supply all of this and a
/// factoriser besides, and this does not use it. What the rank formula needs
/// from a factorisation is the *degrees* of the prime powers and nothing else,
/// and those come from a squarefree decomposition and a distinct-degree split,
/// both deterministic. Givaro's factoriser is Cantor-Zassenhaus, which is
/// randomised. It is a Las Vegas algorithm, so the answer would be the same
/// either way; what would not be the same is the time it took, and this
/// repository publishes timings and reproduces them.
using Polynomial = std::vector<int64_t>;

bool is_zero(const Polynomial& polynomial);

/// The degree. The zero polynomial reports 0 rather than anything cleverer,
/// because every caller here tests `is_zero` first and a signed degree would
/// only move the test.
std::size_t degree(const Polynomial& polynomial);

/// `x^exponent`.
Polynomial monomial(std::size_t exponent);

/// The constant polynomial 1.
Polynomial one();

Polynomial add(const ModularField& field, const Polynomial& left, const Polynomial& right);
Polynomial subtract(const ModularField& field, const Polynomial& left, const Polynomial& right);
Polynomial multiply(const ModularField& field, const Polynomial& left, const Polynomial& right);

/// `left = quotient * right + remainder`, with `remainder` of lower degree than
/// `right`. `right` must not be zero.
void divide(const ModularField& field, const Polynomial& left, const Polynomial& right,
            Polynomial& quotient, Polynomial& remainder);

Polynomial remainder_of(const ModularField& field, const Polynomial& left,
                        const Polynomial& right);

/// The monic greatest common divisor, zero only when both arguments are.
Polynomial greatest_common_divisor(const ModularField& field, const Polynomial& left,
                                   const Polynomial& right);

/// The same polynomial scaled to leading coefficient 1. The zero polynomial is
/// returned unchanged.
Polynomial made_monic(const ModularField& field, const Polynomial& polynomial);

/// The formal derivative, which in characteristic p is zero exactly on the
/// p-th powers. That is not a defect to work around but the fact the squarefree
/// decomposition is built on.
Polynomial derivative(const ModularField& field, const Polynomial& polynomial);

/// `base^exponent mod modulus`, by repeated squaring.
Polynomial power_modulo(const ModularField& field, const Polynomial& base, std::uint64_t exponent,
                        const Polynomial& modulus);

/// The p-th root of a polynomial that is a p-th power.
///
/// Over GF(p) the Frobenius map is an automorphism, so `f(x)^p = f(x^p)`: the
/// coefficients are already their own p-th roots and only the exponents move.
/// The caller must have established that every nonzero coefficient sits at a
/// degree divisible by p, which is what having zero derivative gives it.
Polynomial characteristic_root(const ModularField& field, const Polynomial& polynomial);

}  // namespace pencil_rank
