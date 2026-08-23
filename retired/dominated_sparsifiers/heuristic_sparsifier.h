#pragma once

#include "types.h"

/// The row-basis heuristic: a construction using an invertible block of rows, and
/// the one that produced the only measured numbers this strand has.
///
/// It guarantees the shape of its answer, not its optimality. Nothing here
/// proves a result minimal, and the two [oracles](oracle_sparsifier.h) exist
/// because this does not.
namespace matrix_sparsification {

/// Take a square block of rows of `u` that is invertible and use its inverse.
///
/// As many rows of the result as `u` has columns come out singletons, which is
/// why the construction works at all, and it costs one inversion per subset of
/// rows. Returns the sparsifier `V`, or the identity when nothing improves on
/// doing nothing.
Matrix row_basis_sparsifier(const Field& field, const Matrix& u);

}  // namespace matrix_sparsification
