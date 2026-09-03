#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "row_space_coordinates.h"

/// Breaking a matrix into rank-one pieces.
///
/// A rank-one bilinear form is one multiplication, so this is what turns a map
/// into the products that compute it, and it is where both strands meet the
/// thing they are actually counting.
namespace linear_algebra {

/// Write `matrix` as a sum of exactly rank(matrix) rank-one matrices.
template <class Field>
std::vector<MatrixOver<Field>> rank_one_decomposition(const Field& field,
                                                      const MatrixOver<Field>& matrix) {
    // A maximal independent set of rows, and every row over it: their span is the
    // row space, so every row is a combination of them. The walk is
    // [`coordinates_over_independent_rows`](row_space_coordinates.h), shared with
    // the tensor compression, which wants the same coefficients as a change of
    // basis rather than as outer products.
    std::vector<std::size_t> chosen;
    const MatrixOver<Field> coordinates = coordinates_over_independent_rows(field, matrix, chosen);

    // matrix == coordinates * the chosen rows, so term j is column j of the
    // coordinates against chosen row j: an outer product, hence rank one.
    std::vector<MatrixOver<Field>> terms(chosen.size(),
                                         MatrixOver<Field>(matrix.rows(), matrix.columns()));
    for (std::size_t term = 0; term < chosen.size(); ++term) {
        for (std::size_t row = 0; row < matrix.rows(); ++row) {
            if (field.isZero(coordinates(row, term))) continue;
            for (std::size_t column = 0; column < matrix.columns(); ++column) {
                field.axpyin(terms[term](row, column), coordinates(row, term),
                             matrix(chosen[term], column));
            }
        }
    }
    return terms;
}

}  // namespace linear_algebra
