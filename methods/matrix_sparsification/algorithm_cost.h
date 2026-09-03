#pragma once

#include <cstddef>

#include "types.h"

/// What a fast multiplication algorithm actually costs, once its operators are
/// fixed.
///
/// This is the reason the rest of this folder exists. Sparsifying an operator
/// is not an end in itself: it lowers the additive complexity, which lowers the
/// leading coefficient of the algorithm's arithmetic complexity, which is what
/// decides whether a sub-cubic algorithm beats the classical one on a matrix
/// small enough to care about. An algorithm with a wonderful exponent and a
/// leading coefficient of 100 is a table of asymptotics, not a program.
///
/// `[beniamini2019, Claim 2.11]` for the additive complexities and
/// `[beniamini2019, Claim 3.9]` for what they add up to. Keys are
/// [references.md](../references.md).
namespace matrix_sparsification {

/// The additive complexity of an encoding operator, `nnz + nns - rows`.
///
/// Each row is a linear combination: the first nonzero costs nothing, every
/// later one costs an addition, and an entry that is not a sign costs a
/// multiplication on top. So the two counts this repository keeps are exactly
/// the two terms here, which is why `nns` had to exist before this could.
std::size_t encoding_complexity(const Field& field, const Matrix& operator_);

/// The same for the decoding operator, `nnz + nns - columns`.
///
/// Columns rather than rows because a column of `W` is what combines the
/// products into one entry of the result. Getting this wrong costs exactly
/// `rows - columns` and silently changes the leading coefficient, so it is
/// spelled as its own function rather than left to a flag.
std::size_t decoding_complexity(const Field& field, const Matrix& operator_);

/// The leading coefficient of an `<n, m, k; t>` algorithm's arithmetic
/// complexity, as an exact rational.
///
/// Claim 3.9: `q_u/(t - nm) + q_v/(t - mk) + q_w/(t - nk) + 1`. For a square
/// `<n, n, n; t>` algorithm this is Corollary 3.10's `q/(t - n^2) + 1`, and
/// Strassen is the standard check: `q = 18`, `t = 7`, `n = 2` gives 7,
/// which is the 7 in `7*N^log2(7) - 6*N^2`. Winograd's 15 additions give 6.
///
/// Exact, because it is a ratio of small integers and the whole point is to
/// compare it against 2.
Element leading_coefficient(const Field& field, std::size_t encoding_left,
                            std::size_t encoding_right, std::size_t decoding, std::size_t products,
                            std::size_t rows, std::size_t inner, std::size_t columns);

}  // namespace matrix_sparsification
