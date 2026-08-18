#include "oracle_sparsifier.h"

#include "combinations.h"
#include "matrix_ops.h"
#include "measures.h"
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

namespace {

/// The vector the validator stands for: its combination applied to the rows.
std::vector<Element> combined_row(const Field& field, const Matrix& rows,
                                  const std::vector<Element>& combination) {
    std::vector<Element> result(rows.columns(), Element());
    for (std::size_t row = 0; row < rows.rows(); ++row) {
        if (field.isZero(combination[row])) continue;
        for (std::size_t column = 0; column < rows.columns(); ++column) {
            field.axpyin(result[column], combination[row], rows(row, column));
        }
    }
    return result;
}

std::size_t zero_count(const Field& field, const std::vector<Element>& entries) {
    std::size_t zeros = 0;
    for (const Element& entry : entries) {
        if (field.isZero(entry)) ++zeros;
    }
    return zeros;
}

void write_row(Matrix& rows, std::size_t index, const std::vector<Element>& entries) {
    for (std::size_t column = 0; column < rows.columns(); ++column) {
        rows(index, column) = entries[column];
    }
}

}  // namespace

Matrix sparsify_by_best_corank_one(const Field& field, Matrix rows) {
    if (rows.rows() == 0) return rows;

    // Column subsets one smaller than the number of rows: any larger and the
    // only vector orthogonal to all of them is zero.
    std::vector<std::vector<std::size_t>> viable;
    for (const std::vector<std::size_t>& columns : combinations(rows.columns(), rows.rows() - 1)) {
        if (linear_algebra::rank(field, linear_algebra::select_columns<Field>(rows, columns)) == rows.rows() - 1) {
            viable.push_back(columns);
        }
    }

    std::vector<std::size_t> settled;
    for (std::size_t round = 0; round < rows.rows(); ++round) {
        std::size_t best_zeros = 0;
        std::vector<Element> best_row;
        std::size_t best_index = 0;

        for (const std::vector<std::size_t>& columns : viable) {
            const Validator validator = find_validator(field, rows, columns, settled);
            if (!validator.found) continue;
            const std::vector<Element> vector = combined_row(field, rows, validator.combination);
            const std::size_t zeros = zero_count(field, vector);
            if (zeros > best_zeros) {
                best_zeros = zeros;
                best_row = vector;
                best_index = validator.replaces;
            }
        }

        if (best_row.empty()) break;
        write_row(rows, best_index, best_row);
        settled.push_back(best_index);
    }
    return rows;
}

Matrix sparsify_by_descending_support(const Field& field, Matrix rows) {
    if (rows.rows() == 0) return rows;

    std::vector<std::size_t> settled;
    for (std::size_t round = 0; round < rows.rows(); ++round) {
        bool replaced = false;

        // Largest column set first: a vector forced to zero on more columns
        // cannot be beaten by one forced on fewer, so the first hit is the best
        // and there is nothing to gain by looking further.
        for (std::size_t size = rows.columns() - 1; size + 1 >= rows.rows() && size > 0 && !replaced;
             --size) {
            for (const std::vector<std::size_t>& columns : combinations(rows.columns(), size)) {
                const Validator validator = find_validator(field, rows, columns, settled);
                if (!validator.found) continue;
                const std::vector<Element> vector =
                    combined_row(field, rows, validator.combination);
                if (zero_count(field, vector) == 0) continue;

                write_row(rows, validator.replaces, vector);
                settled.push_back(validator.replaces);
                replaced = true;
                break;
            }
        }
        if (!replaced) break;
    }
    return rows;
}

}  // namespace matrix_sparsification
