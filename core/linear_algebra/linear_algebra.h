#pragma once

/// The shared exact-arithmetic layer, as one include for callers who want all
/// of it. There is no code here: each part is a file named for its own role,
/// and this header only says which parts exist.
///
/// Include the part you need instead when you need one. The umbrella exists so
/// that a caller reaching for "linear algebra" gets it, not so that everything
/// arrives everywhere.
///
/// - [`field.h`](field.h)                 the two fields and the matrix aliases
/// - [`matrix.h`](matrix.h)               the dense matrix
/// - [`span_basis.h`](span_basis.h)       a span kept in echelon form, and spans of slices
/// - [`measures.h`](measures.h)           rank, multiplications, nonzeros
/// - [`span_queries.h`](span_queries.h)   what a span contains, and the invariants
/// - [`solver.h`](solver.h)               exact solve, and inversion
/// - [`row_space_coordinates.h`](row_space_coordinates.h) every row over an independent subset of them
/// - [`matrix_ops.h`](matrix_ops.h)       transpose, product, row and column selection
/// - [`decomposition.h`](decomposition.h) a matrix as rank-one pieces
/// - [`tensor_flattening.h`](tensor_flattening.h) the three flattenings, and the rank bound they give
/// - [`rank_decomposition.h`](rank_decomposition.h) a tensor as rank-one terms, and the tensor they rebuild
/// - [`tensor_compression.h`](tensor_compression.h) the concise core, and a decomposition of it carried back

#include "decomposition.h"
#include "field.h"
#include "matrix.h"
#include "matrix_ops.h"
#include "measures.h"
#include "rank_decomposition.h"
#include "row_space_coordinates.h"
#include "solver.h"
#include "span_basis.h"
#include "span_queries.h"
#include "tensor_compression.h"
#include "tensor_flattening.h"
