#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"

/// The rank of a tensor written as a factorisation over the canonical basis.
///
/// Given `T` with slices in `F^{n x m}`, take the canonical basis `B` of that
/// space, the `nm` matrices with a single 1. Ask for a matrix `A` such that the
/// rows of `A B` span a space containing the slices, and such that `A` has as
/// few rows as possible.
///
/// `B` is where the formulation is clearest and also where it contributes
/// nothing: `A B` is the reading map, taking a row of coefficients to the
/// matrix whose entries they are, so **every** matrix list is `A B` for some
/// `A`. The whole content is the constraint that each row of `A` must read as a
/// **rank-one** matrix, and with that constraint the least number of rows is
/// exactly the rank of `T`. See [`README.md`](README.md) for the equivalence and
/// for whose definition this is.
namespace canonical_factorisation {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

/// The canonical basis of the `rows` by `columns` matrix space, in the row-major
/// order that `matrix_of` reads back.
std::vector<ModularMatrix> canonical_basis(std::size_t rows, std::size_t columns);

/// Row `index` of `coefficients` read against that basis, which is the matrix
/// whose entries the row holds. This is one row of the product `A B`.
ModularMatrix matrix_of(const ModularMatrix& coefficients, std::size_t index, std::size_t rows,
                        std::size_t columns);

/// The `k` by `nm` matrix whose rows are the slices flattened: the `S` that
/// `C A` has to reproduce exactly.
ModularMatrix slice_matrix(const std::vector<ModularMatrix>& slices);

}  // namespace canonical_factorisation
