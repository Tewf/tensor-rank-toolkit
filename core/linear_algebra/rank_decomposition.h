#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "field.h"
#include "matrix.h"

/// A rank decomposition of a tensor, and the tensor it rebuilds.
///
/// `T = sum_j a_j (x) b_j (x) c_j`, held as three factor matrices with one row
/// per term: row `j` of `factor_by_axis[d]` is that term's vector along axis `d`,
/// so factor `d` is `terms x n_d`. The axes are the ones
/// [`tensor_contraction.h`](tensor_contraction.h) fixes: 0 rows, 1 columns, 2
/// slices.
///
/// **The rank is the smallest number of terms**, so a decomposition is what an
/// upper bound on the rank consists of, and the count is the only thing about one
/// that a search reports. Which is why the operation this type exists for,
/// [`expand_decomposition`](tensor_compression.h), is judged by leaving the count
/// alone.
///
/// The same object exists a layer up, as `Algorithm` in
/// [`../../methods/bilinear_rank/algorithm_recovery.h`](../../methods/bilinear_rank/algorithm_recovery.h):
/// `left` and `right` are the axis 0 and axis 1 factors as they stand, and
/// `decode` is the axis 2 factor transposed, because an algorithm is read by
/// output and a decomposition by term. Nothing converts between them here. This
/// layer may not depend on that one, and the conversion is a transpose.
namespace linear_algebra {

template <class Field>
struct RankDecomposition {
    /// Three factors, axis 0 first. Each has one row per term.
    std::vector<MatrixOver<Field>> factor_by_axis;

    /// What a search minimises, and what carrying a decomposition must preserve.
    std::size_t term_count() const {
        return factor_by_axis.empty() ? 0 : factor_by_axis.front().rows();
    }
};

/// The tensor a decomposition computes, as slices, so it can be compared with the
/// tensor it is meant to decompose.
///
/// Nothing here trusts a decomposition it was handed: a set of factors that does
/// not rebuild the tensor is not a cheaper decomposition of it, it is a wrong
/// one, and the only way to see that is to multiply it out. The shapes come from
/// the factors' widths, so a decomposition carries the shape of the tensor it is
/// of and cannot be silently applied to another.
template <class Field>
std::vector<MatrixOver<Field>> tensor_from_decomposition(
    const Field& field, const RankDecomposition<Field>& decomposition) {
    using Element = typename Field::Element;
    if (decomposition.factor_by_axis.size() != 3) {
        throw std::invalid_argument("a decomposition is three factors, one per axis");
    }
    const MatrixOver<Field>& rows_factor = decomposition.factor_by_axis[0];
    const MatrixOver<Field>& columns_factor = decomposition.factor_by_axis[1];
    const MatrixOver<Field>& slices_factor = decomposition.factor_by_axis[2];
    if (columns_factor.rows() != rows_factor.rows() || slices_factor.rows() != rows_factor.rows()) {
        throw std::invalid_argument("every factor of a decomposition carries one row per term");
    }

    std::vector<MatrixOver<Field>> slices(
        slices_factor.columns(),
        MatrixOver<Field>(rows_factor.columns(), columns_factor.columns()));
    for (std::size_t term = 0; term < rows_factor.rows(); ++term) {
        for (std::size_t slice = 0; slice < slices_factor.columns(); ++slice) {
            if (field.isZero(slices_factor(term, slice))) continue;
            for (std::size_t row = 0; row < rows_factor.columns(); ++row) {
                Element weight;
                field.mul(weight, slices_factor(term, slice), rows_factor(term, row));
                if (field.isZero(weight)) continue;
                for (std::size_t column = 0; column < columns_factor.columns(); ++column) {
                    field.axpyin(slices[slice](row, column), weight, columns_factor(term, column));
                }
            }
        }
    }
    return slices;
}

}  // namespace linear_algebra
