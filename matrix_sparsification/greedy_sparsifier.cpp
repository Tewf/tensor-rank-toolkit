#include "greedy_sparsifier.h"

#include "combinations.h"
#include "matrix_ops.h"
#include "measures.h"
#include "oracle_sparsifier.h"

namespace matrix_sparsification {

namespace {

/// How many entries are neither zero nor a sign, for a single vector.
std::size_t nonsign_count(const Field& field, const std::vector<Element>& entries) {
    std::size_t total = 0;
    for (const Element& entry : entries) {
        if (field.isZero(entry) || field.isOne(entry) || field.isMOne(entry)) continue;
        ++total;
    }
    return total;
}

std::size_t zero_count(const Field& field, const std::vector<Element>& entries) {
    std::size_t zeros = 0;
    for (const Element& entry : entries) {
        if (field.isZero(entry)) ++zeros;
    }
    return zeros;
}

std::vector<Element> scaled_by(const Field& field, const std::vector<Element>& vector,
                               const Element& factor) {
    std::vector<Element> result(vector.size());
    for (std::size_t index = 0; index < vector.size(); ++index) {
        field.mul(result[index], vector[index], factor);
    }
    return result;
}

/// The combination the validator stands for, applied to the rows.
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

}  // namespace

std::vector<Element> best_scaling(const Field& field, const std::vector<Element>& vector) {
    std::vector<Element> best = vector;
    std::size_t fewest = nonsign_count(field, vector);
    if (fewest == 0) return best;

    for (const Element& entry : vector) {
        if (field.isZero(entry)) continue;

        Element factor;
        field.inv(factor, entry);
        for (int sign = 0; sign < 2; ++sign) {
            if (sign == 1) field.negin(factor);

            const std::vector<Element> candidate = scaled_by(field, vector, factor);
            const std::size_t score = nonsign_count(field, candidate);
            if (score < fewest) {
                fewest = score;
                best = candidate;
            }
        }
    }
    return best;
}

Matrix greedy_sparsify(const Field& field, Matrix rows) {
    if (rows.rows() == 0) return rows;

    // The article's objective is nnz + nns, so a vector is better when it has
    // more zeros, and among equal zero counts when fewer of what is left needs
    // a multiplication rather than an addition.
    const auto better = [](std::size_t zeros, std::size_t nonsigns, std::size_t best_zeros,
                           std::size_t best_nonsigns) {
        if (zeros != best_zeros) return zeros > best_zeros;
        return nonsigns < best_nonsigns;
    };

    std::vector<std::size_t> settled;
    for (std::size_t round = 0; round < rows.rows(); ++round) {
        std::size_t best_zeros = 0;
        std::size_t best_nonsigns = rows.columns() + 1;
        std::vector<Element> best_row;
        std::size_t best_index = 0;

        for (const std::vector<std::size_t>& columns : combinations(rows.columns(), rows.rows() - 1)) {
            const Validator validator = find_validator(field, rows, columns, settled);
            if (!validator.found) continue;

            const std::vector<Element> vector =
                best_scaling(field, combined_row(field, rows, validator.combination));
            const std::size_t zeros = zero_count(field, vector);
            const std::size_t nonsigns = nonsign_count(field, vector);
            if (zeros == 0) continue;

            if (best_row.empty() || better(zeros, nonsigns, best_zeros, best_nonsigns)) {
                best_zeros = zeros;
                best_nonsigns = nonsigns;
                best_row = vector;
                best_index = validator.replaces;
            }
        }

        if (best_row.empty()) break;
        for (std::size_t column = 0; column < rows.columns(); ++column) {
            rows(best_index, column) = best_row[column];
        }
        settled.push_back(best_index);
    }
    return rows;
}

}  // namespace matrix_sparsification
