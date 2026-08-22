#include "omega_validator.h"

#include "linear_algebra.h"
#include "matrix_ops.h"
#include "solver.h"

namespace matrix_sparsification {

Validator find_validator(const Field& field, const Matrix& rows,
                         const std::vector<std::size_t>& columns,
                         const std::vector<std::size_t>& settled) {
    const Matrix restricted = linear_algebra::select_columns<Field>(rows, columns);

    for (std::size_t candidate = 0; candidate < rows.rows(); ++candidate) {
        bool already_settled = false;
        for (std::size_t index : settled) already_settled |= (index == candidate);
        if (already_settled) continue;

        // Is this row, on these columns, in the span of the other rows?
        std::vector<std::vector<Element>> others;
        std::vector<std::size_t> other_indices;
        for (std::size_t row = 0; row < restricted.rows(); ++row) {
            if (row == candidate) continue;
            others.push_back(restricted.row(row));
            other_indices.push_back(row);
        }

        std::vector<Element> coefficients;
        if (!linear_algebra::solve_in_row_space(field, others, restricted.row(candidate), coefficients)) {
            continue;
        }

        // combination . rows == 0 on `columns`, with -1 in the candidate's place.
        Validator validator;
        validator.found = true;
        validator.replaces = candidate;
        validator.combination.assign(rows.rows(), Element());
        for (std::size_t index = 0; index < other_indices.size(); ++index) {
            validator.combination[other_indices[index]] = coefficients[index];
        }
        field.neg(validator.combination[candidate], field.one);
        return validator;
    }
    return Validator();
}

}  // namespace matrix_sparsification
