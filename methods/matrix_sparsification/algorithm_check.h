#pragma once

#include <cstddef>

#include "types.h"

/// Whether a triple of operators computes the matrix product it claims to.
///
/// Needed because everything else here rewrites operators. Sparsifying,
/// rescaling and decomposing all preserve the algorithm in theory; this is what
/// says they did in practice. `same_row_space` checks that a sparsified
/// operator spans what it used to, which is the right check for one operator on
/// its own, but it cannot see whether the three of them together still multiply
/// matrices.
///
/// The test is the trilinear identity behind `<n, m, k; t>`: summing
/// `U[r][p]·V[r][q]·W[r][o]` over the `t` products must give one exactly when
/// the indices line up as `a_ij · b_jl` contributing to `c_il`, and zero
/// otherwise. `[beniamini2019, Def. 2.8]`.
namespace matrix_sparsification {

/// The operators of an `<n, m, k; t>` algorithm: `U` is `t x nm`, `V` is
/// `t x mk`, `W` is `t x nk`, all indexed row-major over the operands.
///
/// [`bilinear_rank::Algorithm`](../bilinear_rank/algorithm_recovery.h) is the
/// same object over a modular field with **`W` transposed**: its `decode` is
/// outputs x products where this `decoding` is products x outputs, each
/// following its own source's convention. They stay two types because merging
/// them would either transpose a cited definition or leave a member comment
/// that lies for one of the two.
struct BilinearAlgorithm {
    Matrix left_encoding;
    Matrix right_encoding;
    Matrix decoding;
};

/// True when the triple computes the `n x m` by `m x k` product exactly.
///
/// Shapes are checked first, so a triple that cannot possibly be an algorithm
/// of that shape is a false rather than an out-of-range read.
bool computes_matrix_product(const Field& field, const BilinearAlgorithm& algorithm,
                             std::size_t rows, std::size_t inner, std::size_t columns);

}  // namespace matrix_sparsification
