#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"

/// Building one matrix out of another: transpose, product, and the two
/// selections the sparsification restricts operators with.
namespace linear_algebra {

template <class Field>
MatrixOver<Field> transpose(const MatrixOver<Field>& matrix) {
    MatrixOver<Field> result(matrix.columns(), matrix.rows());
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            result(column, row) = matrix(row, column);
        }
    }
    return result;
}

template <class Field>
MatrixOver<Field> multiply(const Field& field, const MatrixOver<Field>& left,
                           const MatrixOver<Field>& right) {
    MatrixOver<Field> result(left.rows(), right.columns());
    for (std::size_t row = 0; row < left.rows(); ++row) {
        for (std::size_t inner = 0; inner < left.columns(); ++inner) {
            if (field.isZero(left(row, inner))) continue;
            for (std::size_t column = 0; column < right.columns(); ++column) {
                field.axpyin(result(row, column), left(row, inner), right(inner, column));
            }
        }
    }
    return result;
}

/// The columns named by `chosen`, in the order given.
template <class Field>
MatrixOver<Field> select_columns(const MatrixOver<Field>& matrix,
                                 const std::vector<std::size_t>& chosen) {
    MatrixOver<Field> result(matrix.rows(), chosen.size());
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < chosen.size(); ++column) {
            result(row, column) = matrix(row, chosen[column]);
        }
    }
    return result;
}

/// The rows named by `chosen`, in the order given.
template <class Field>
MatrixOver<Field> select_rows(const MatrixOver<Field>& matrix,
                              const std::vector<std::size_t>& chosen) {
    MatrixOver<Field> result(chosen.size(), matrix.columns());
    for (std::size_t row = 0; row < chosen.size(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            result(row, column) = matrix(chosen[row], column);
        }
    }
    return result;
}

}  // namespace linear_algebra
