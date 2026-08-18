#include "binary_encoding.h"

#include "symmetry_breaking.h"

#include <stdexcept>
#include <string>

namespace satisfiability {

namespace {

std::size_t budget = 10'000'000;

/// `result ↔ left ∧ right`, in the three clauses that says.
void equate_to_conjunction(linear_algebra::Cnf& formula, int result, int left, int right) {
    formula.add_clause({-result, left});
    formula.add_clause({-result, right});
    formula.add_clause({result, -left, -right});
}

std::vector<int> term_variables(const BinaryEncoding& encoding, std::size_t term) {
    std::vector<int> all;
    all.reserve(encoding.rows + encoding.columns + encoding.slices);
    for (std::size_t index = 0; index < encoding.rows; ++index) {
        all.push_back(encoding.left[term * encoding.rows + index]);
    }
    for (std::size_t index = 0; index < encoding.columns; ++index) {
        all.push_back(encoding.right[term * encoding.columns + index]);
    }
    for (std::size_t index = 0; index < encoding.slices; ++index) {
        all.push_back(encoding.output[term * encoding.slices + index]);
    }
    return all;
}

}  // namespace

std::size_t variable_budget() { return budget; }
void set_variable_budget(std::size_t variables) { budget = variables; }

BinaryEncoding encode_rank_at_most(const linear_algebra::Tensor& tensor, std::size_t products,
                                   bool break_symmetry, bool first_term_pinned) {
    if (tensor.characteristic != 2) {
        throw std::invalid_argument(
            "the Boolean encoding is GF(2) only, and this tensor is over GF(" +
            std::to_string(tensor.characteristic) + "); use the prime-field encoding");
    }
    if (tensor.slices.empty()) throw std::invalid_argument("a tensor with no slices has rank 0");

    BinaryEncoding encoding;
    encoding.products = products;
    encoding.rows = tensor.rows();
    encoding.columns = tensor.columns();
    encoding.slices = tensor.slices.size();

    const std::size_t entries = encoding.rows * encoding.columns * encoding.slices;
    if (products != 0 && entries > budget / products) {
        throw std::invalid_argument(
            "encoding " + std::to_string(products) + " products of a " +
            std::to_string(encoding.rows) + "x" + std::to_string(encoding.columns) + "x" +
            std::to_string(encoding.slices) + " tensor wants " + std::to_string(products) + "*" +
            std::to_string(entries) + " product variables, over a budget of " +
            std::to_string(budget));
    }

    linear_algebra::Cnf& formula = encoding.formula;
    for (std::size_t term = 0; term < products; ++term) {
        for (std::size_t index = 0; index < encoding.rows; ++index) {
            encoding.left.push_back(formula.new_variable());
        }
        for (std::size_t index = 0; index < encoding.columns; ++index) {
            encoding.right.push_back(formula.new_variable());
        }
        for (std::size_t index = 0; index < encoding.slices; ++index) {
            encoding.output.push_back(formula.new_variable());
        }
    }

    // q[l][i][j] is a[l][i] and b[l][j], shared across every slice.
    std::vector<int> pair(products * encoding.rows * encoding.columns, 0);
    for (std::size_t term = 0; term < products; ++term) {
        for (std::size_t row = 0; row < encoding.rows; ++row) {
            for (std::size_t column = 0; column < encoding.columns; ++column) {
                const int product = formula.new_variable();
                equate_to_conjunction(formula, product, encoding.left[term * encoding.rows + row],
                                      encoding.right[term * encoding.columns + column]);
                pair[(term * encoding.rows + row) * encoding.columns + column] = product;
            }
        }
    }

    // One parity constraint per entry of the tensor.
    for (std::size_t row = 0; row < encoding.rows; ++row) {
        for (std::size_t column = 0; column < encoding.columns; ++column) {
            for (std::size_t slice = 0; slice < encoding.slices; ++slice) {
                std::vector<int> summands;
                summands.reserve(products);
                for (std::size_t term = 0; term < products; ++term) {
                    const int triple = formula.new_variable();
                    equate_to_conjunction(
                        formula, triple, pair[(term * encoding.rows + row) * encoding.columns + column],
                        encoding.output[term * encoding.slices + slice]);
                    summands.push_back(triple);
                }
                const bool wanted = !Field(2).isZero(tensor.slices[slice](row, column));
                formula.add_parity(std::move(summands), wanted);
            }
        }
    }

    if (break_symmetry) {
        for (std::size_t term = first_term_pinned ? 1 : 0; term + 1 < products; ++term) {
            order_lexicographically(formula, term_variables(encoding, term),
                                    term_variables(encoding, term + 1));
        }
    }
    return encoding;
}

}  // namespace satisfiability
