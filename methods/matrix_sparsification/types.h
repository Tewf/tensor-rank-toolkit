#pragma once

#include "field.h"

/// Matrix sparsification: given the operator `U` of a fast multiplication
/// algorithm, find an invertible `V` minimising `nnz(U V)`.
///
/// Fewer nonzeros in the operator means fewer additions in the algorithm it
/// encodes, which is the cost the multiplication count does not capture. The
/// filenames say what each route guarantees:
///
/// - [`rational_sparsifier.h`](rational_sparsifier.h): the **minimum** over
///   every invertible `V`, by the matroid greedy.
/// - [`lightest_vector_by_simplex.h`](lightest_vector_by_simplex.h): an upper
///   bound, by linear programming, and the only route that answers an operator
///   too large to search.
/// - [`greedy_sparsifier.h`](greedy_sparsifier.h): no guarantee, and the only
///   one minimising `nnz + nns` rather than `nnz`.
/// - [`finite_field_sparsifier.h`](finite_field_sparsifier.h): the minimum
///   again, over a finite field, where the space can simply be walked.
namespace matrix_sparsification {

/// Everything here works in exact rationals. The entries of a real operator are
/// fractions like 4/9, which no double holds, and the quantity being minimised
/// is how many of them are zero: a value that rounds to zero is not nearly
/// right, it is a different answer.
using Field = linear_algebra::RationalField;
using Matrix = linear_algebra::RationalMatrix;
using Element = Field::Element;

}  // namespace matrix_sparsification
