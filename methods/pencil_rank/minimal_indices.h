#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"

/// Exact rank of a two-slice tensor, in polynomial time.
///
/// Every other search in this repository is exponential because it is looking
/// for a set of rank-one maps. For two slices nobody has to look: the rank is
/// determined by the Kronecker canonical form of the matrix pencil the two
/// slices are, and that form is computed by exact linear algebra.
///
/// The structure theorem is `[gantmacher1959, Ch. XII §5, Thm. 5]`: two pencils
/// of the same shape are strictly equivalent **if and only if** they have the
/// same minimal indices and the same finite and infinite elementary divisors.
/// That is what licenses computing the four counts and never assembling the
/// form. The canonical form itself is `[gantmacher1959, Ch. XII §4, (30)]`,
/// restated modernly as `[sumi2009, Lem. 2.1]`. Keys are
/// [`../references.md`](../references.md); [`README.md`](README.md) is what the
/// rank formula then does with them.
namespace pencil_rank {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

/// The column minimal indices of the pencil `first + x second`, ascending.
///
/// These are the degrees of a minimal basis of the pencil's null space over
/// `GF(p)(x)`, and they are read off ranks rather than computed as vectors. A
/// null vector of degree below `depth` is a solution of the block system
///
///     [ A          ] [ v_0     ]
///     [ B  A       ] [ v_1     ] = 0
///     [    B  ...  ] [ ...     ]
///     [        ... ] [ v_{d-1} ]
///
/// whose matrix is `(depth + 1) n` by `depth m`, so the nullity of that matrix
/// is `sum_i max(0, depth - index_i)`. Differencing twice in `depth` recovers
/// how many indices take each value, and no null vector is ever formed.
///
/// `expected_count` is `m - rank(A + xB)` over `GF(p)(x)`, which the caller
/// already holds from the diagonal form. Passing it in is what stops the scan
/// the moment every index is accounted for, rather than at the worst-case bound
/// of `m`: a regular pencil has none and stops at the first step. Disagreeing
/// with it is a defect and throws rather than returning a plausible answer.
std::vector<std::size_t> column_minimal_indices(const ModularField& field,
                                                const ModularMatrix& first,
                                                const ModularMatrix& second,
                                                std::size_t expected_count);

/// The row minimal indices, which are the column minimal indices of the
/// transposed pencil. Kronecker's form is symmetric under transposition and so
/// is this, which is why there is one implementation and not two:
/// `[gantmacher1959, Ch. XII §5]` defines the row indices `η_j` as the column
/// indices of `Aᵀ + xBᵀ` and nothing else.
std::vector<std::size_t> row_minimal_indices(const ModularField& field,
                                             const ModularMatrix& first,
                                             const ModularMatrix& second,
                                             std::size_t expected_count);

}  // namespace pencil_rank
