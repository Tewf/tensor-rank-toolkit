#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

/// Building the bilinear maps the search then decomposes.
///
/// Two key cases: multiplying polynomials, and multiplying elements of GF(pⁿ),
/// which is the first case, reduced modulo an irreducible polynomial.
namespace bilinear_rank {

/// Coefficients highest degree first: `1 0 3` represents `x² + 3`.
using Polynomial = std::vector<Element>;

/// The tensor of multiplying a polynomial of `left_terms` coefficients by one
/// of `right_terms`.
///
/// Slice `i` carries a 1 at `(j, k)` exactly when `j + k == i`, and there are
/// `left_terms + right_terms - 1` of them, in ascending degree. Computing every
/// product separately costs `left_terms * right_terms`, which is the rank to
/// beat.
std::vector<Matrix> polynomial_multiplication_tensor(std::size_t left_terms,
                                                     std::size_t right_terms);

/// Whether `modulus` is irreducible over GF(p), so `GF(p)[x]/(modulus)` is a
/// field.
///
/// Uses Givaro's polynomial domain to test irreducibility, avoiding the sympy
/// dependency that earlier approaches required.
bool is_irreducible(const Field& field, const Polynomial& modulus);

/// Euclidean division of a polynomial whose coefficients are matrices by one
/// whose coefficients are scalars, keeping the remainder.
///
/// `slices` are in descending degree here, matching how `modulus` is written.
/// If `slices` is already below the modulus's degree it comes back unchanged;
/// otherwise zero coefficients are dropped from the result, so it is then
/// shorter than what went in. Throws if `modulus` is empty or its leading
/// coefficient is zero.
std::vector<Matrix> reduce_tensor_modulo(const Field& field, std::vector<Matrix> slices,
                                         const Polynomial& modulus);

/// The tensor of multiplying an `n x m` matrix by an `m x k` one: the tensor
/// the complexity literature calls ⟨n, m, k⟩.
///
/// The left operand is `A` read row by row, the right is `B` read row by row,
/// and there is one slice per entry of `C`, so the shape here is `n*k` slices
/// of `(n*m) x (m*k)`. Slice `(i, l)` carries a 1 at `((i,j), (j,l))` for every
/// `j`, which is the sum defining `C[i][l]`.
///
/// ⟨2,2,2⟩ is where fast matrix multiplication starts: Strassen's seven
/// products instead of eight, and Winograd's proof that seven is the floor.
///
/// Throws, via
/// [`run_limits::require_room`](../../../infrastructure/run_limits/memory_budget.h),
/// if the shape does not fit the memory budget.
std::vector<Matrix> matrix_multiplication_tensor(std::size_t rows, std::size_t inner,
                                                 std::size_t columns);

/// The tensor of multiplying two polynomials modulo `x^n - 1`, one of the
/// families for which lower bounds over GF(2) and GF(3) are published.
///
/// Slice `i` carries a 1 at `(j, l)` exactly when `j + l ≡ i (mod n)`, so every
/// slice is a permutation matrix and the naive cost is `n²`.
///
/// Throws, via `run_limits::require_room`, if the shape does not fit the
/// memory budget.
std::vector<Matrix> cyclic_convolution_tensor(std::size_t length);

/// The tensor of multiplication in GF(pⁿ), for `modulus` of degree `n`:
/// multiply two elements as polynomials, then reduce.
///
/// Builds the polynomial product using the degree, so both operands are reduced
/// field elements. Throws if `modulus` is not irreducible over GF(p).
std::vector<Matrix> field_multiplication_tensor(const Field& field, const Polynomial& modulus);

}  // namespace bilinear_rank
