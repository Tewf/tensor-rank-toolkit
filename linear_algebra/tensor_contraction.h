#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "field.h"
#include "matrix.h"

/// Collapsing one axis of a tensor into a single linear combination, which
/// turns the tensor back into an ordinary matrix.
///
/// A tensor here is a list of slices indexed as `T(row, column, slice)`, the
/// convention [`tensor_flattening.h`](tensor_flattening.h) already uses: axis 0
/// is rows, axis 1 is columns, axis 2 is the slice index. Contracting along axis
/// `d` by a vector `v` of length `n_d` gives the matrix on the two remaining
/// axes whose entries are `sum_i v[i] * T[... i ...]`.
///
/// A flattening **keeps** an axis whole; a contraction **collapses** it. The
/// difference is why one is weak and the other is not. Rank behaves well under
/// contraction: if `T = sum_r a_r (x) b_r (x) c_r` has `R` terms then
///
/// > `rk(v ·_d T) <= #{r : <v, a_r> != 0}`
///
/// because the terms `v` annihilates contribute nothing. So a contraction rank
/// reports a *count of surviving terms*, and counting those across many `v` at
/// once is exactly what Jason Yang's rank-sum bound does
/// ([`tensor_rank_sum.h`](tensor_rank_sum.h), `[yang2025]`; keys are
/// [`../references.md`](../references.md)).
namespace linear_algebra {

/// The length of `axis`: rows for axis 0, columns for axis 1, the number of
/// slices for axis 2.
template <class Field>
std::size_t axis_dimension(const std::vector<MatrixOver<Field>>& slices, std::size_t axis) {
    if (slices.empty()) return 0;
    if (axis == 0) return slices.front().rows();
    if (axis == 1) return slices.front().columns();
    return slices.size();
}

/// `v ·_axis T`: the matrix left when `axis` is collapsed by `coefficients`.
///
/// The two surviving axes stay in ascending order, so contracting axis 0 gives a
/// columns-by-slices matrix, axis 1 a rows-by-slices matrix, and axis 2 a
/// rows-by-columns matrix. Only the rank of the result is ever used here, and
/// rank does not care which of the two is which, but fixing the order keeps the
/// function testable.
template <class Field>
MatrixOver<Field> contraction(const Field& field, const std::vector<MatrixOver<Field>>& slices,
                              std::size_t axis,
                              const std::vector<typename Field::Element>& coefficients) {
    if (slices.empty()) return MatrixOver<Field>();
    if (coefficients.size() != axis_dimension<Field>(slices, axis)) {
        throw std::invalid_argument("contraction: one coefficient per position along the axis");
    }
    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();
    const std::size_t depth = slices.size();

    MatrixOver<Field> result(axis == 0 ? columns : rows, axis == 2 ? columns : depth);
    for (std::size_t slice = 0; slice < depth; ++slice) {
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t column = 0; column < columns; ++column) {
                const std::size_t position = axis == 0 ? row : (axis == 1 ? column : slice);
                if (field.isZero(coefficients[position])) continue;
                const std::size_t line = axis == 0 ? column : row;
                const std::size_t offset = axis == 2 ? column : slice;
                field.axpyin(result(line, offset), coefficients[position],
                             slices[slice](row, column));
            }
        }
    }
    return result;
}

}  // namespace linear_algebra
