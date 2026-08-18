#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "solver.h"

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
    using Element = typename Field::Element;

    // A maximal independent set of rows, in order: their span is the row space,
    // so every row is a combination of them.
    //
    // Each row's combination is kept as it is found. The solve that decides
    // whether a row is independent already computes it, and against an
    // independent set the answer is unique, so a coefficient found against the
    // basis so far is still the answer against the finished basis with zeros
    // appended. Solving a second time per row, as this did, was asking a
    // question already answered.
    std::vector<std::vector<Element>> basis_rows;
    std::vector<std::vector<Element>> combination_of_row(matrix.rows());
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        std::vector<Element> entries = matrix.row(row);
        std::vector<Element> coefficients;
        if (solve_in_row_space(field, basis_rows, entries, coefficients)) {
            combination_of_row[row] = std::move(coefficients);
            continue;
        }
        // Independent: it becomes a basis row, and is exactly itself.
        combination_of_row[row].assign(basis_rows.size(), Element());
        combination_of_row[row].push_back(Element());
        field.assign(combination_of_row[row].back(), field.one);
        basis_rows.push_back(std::move(entries));
    }

    // matrix == coefficients * basis_rows, so term j is column j of the
    // coefficients against basis row j: an outer product, hence rank one.
    std::vector<MatrixOver<Field>> terms(basis_rows.size(),
                                         MatrixOver<Field>(matrix.rows(), matrix.columns()));
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        const std::vector<Element>& coefficients = combination_of_row[row];
        for (std::size_t term = 0; term < coefficients.size(); ++term) {
            for (std::size_t column = 0; column < matrix.columns(); ++column) {
                field.axpyin(terms[term](row, column), coefficients[term],
                             basis_rows[term][column]);
            }
        }
    }
    return terms;
}

}  // namespace linear_algebra
