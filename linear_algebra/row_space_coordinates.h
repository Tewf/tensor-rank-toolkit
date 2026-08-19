#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "solver.h"

/// Every row of a matrix written over a maximal independent subset of its rows.
///
/// One elimination, two readings, which is why it is its own file rather than a
/// second thing inside [`solver.h`](solver.h): the coefficients are outer
/// products to [`decomposition.h`](decomposition.h) and a change of basis on one
/// axis to [`tensor_compression.h`](tensor_compression.h), and neither reading
/// belongs to the other.
namespace linear_algebra {

/// `chosen` names that subset, in the order the rows appear, and row `i` of the
/// result holds the coefficients that rebuild row `i` from the chosen rows. So
/// `matrix == result * select_rows(matrix, chosen)`, and `result` is the identity
/// on the chosen rows themselves.
///
/// Each row's coefficients are kept as they are found. The solve that decides
/// whether a row is independent already computes them, and against an independent
/// set the answer is unique, so coefficients found against the subset so far are
/// still the answer against the finished subset with zeros appended. Solving a
/// second time per row is asking a question already answered.
template <class Field>
MatrixOver<Field> coordinates_over_independent_rows(const Field& field,
                                                    const MatrixOver<Field>& matrix,
                                                    std::vector<std::size_t>& chosen) {
    using Element = typename Field::Element;

    std::vector<std::vector<Element>> chosen_rows;
    std::vector<std::vector<Element>> coefficients_of_row(matrix.rows());
    chosen.clear();
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        std::vector<Element> entries = matrix.row(row);
        std::vector<Element> coefficients;
        if (solve_in_row_space(field, chosen_rows, entries, coefficients)) {
            coefficients_of_row[row] = std::move(coefficients);
            continue;
        }
        // Independent: it joins the subset, and is exactly itself.
        coefficients_of_row[row].assign(chosen_rows.size(), Element());
        coefficients_of_row[row].emplace_back();
        field.assign(coefficients_of_row[row].back(), field.one);
        chosen.push_back(row);
        chosen_rows.push_back(std::move(entries));
    }

    MatrixOver<Field> coordinates(matrix.rows(), chosen_rows.size());
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t position = 0; position < coefficients_of_row[row].size(); ++position) {
            coordinates(row, position) = coefficients_of_row[row][position];
        }
    }
    return coordinates;
}

}  // namespace linear_algebra
