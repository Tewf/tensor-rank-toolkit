#include "prime_field_encoding.h"

#include <stdexcept>
#include <string>

#include "binary_encoding.h"
#include "symmetry_breaking.h"

namespace satisfiability {

namespace {

/// A fresh group of `size` variables, exactly one of which is true.
///
/// At-least-one is a clause; at-most-one is every pair. Quadratic, and at the
/// primes in play that is three clauses, so a ladder encoding would be more
/// machinery than it saves.
std::vector<int> one_hot(linear_algebra::Cnf& formula, std::size_t size) {
    std::vector<int> group;
    group.reserve(size);
    for (std::size_t value = 0; value < size; ++value) group.push_back(formula.new_variable());

    formula.add_clause(group);
    for (std::size_t first = 0; first < size; ++first) {
        for (std::size_t second = first + 1; second < size; ++second) {
            formula.add_clause({-group[first], -group[second]});
        }
    }
    return group;
}

/// A fresh group holding the field operation of two groups, table and all.
///
/// `linear_combination(x, y)` is the field's own answer, so the same eight lines serve
/// multiplication and addition and there is one place for either to be wrong.
template <class Operation>
std::vector<int> combined(linear_algebra::Cnf& formula, std::size_t characteristic,
                          const std::vector<int>& first, const std::vector<int>& second,
                          Operation linear_combination) {
    const std::vector<int> result = one_hot(formula, characteristic);
    for (std::size_t left = 0; left < characteristic; ++left) {
        for (std::size_t right = 0; right < characteristic; ++right) {
            formula.add_clause(
                {-first[left], -second[right], result[linear_combination(left, right)]});
        }
    }
    return result;
}

/// The one-hot group of the constant zero.
std::vector<int> constant_zero(linear_algebra::Cnf& formula, std::size_t characteristic) {
    const std::vector<int> group = one_hot(formula, characteristic);
    formula.add_clause({group[0]});
    return group;
}

std::vector<int> slice_of(const std::vector<int>& flat, std::size_t offset,
                          std::size_t characteristic) {
    return std::vector<int>(flat.begin() + static_cast<std::ptrdiff_t>(offset * characteristic),
                            flat.begin() +
                                static_cast<std::ptrdiff_t>((offset + 1) * characteristic));
}

/// Every variable of one term, in a fixed order, for the lexicographic
/// constraint to compare against the next term's.
std::vector<int> term_variables(const PrimeFieldEncoding& encoding, std::size_t term) {
    std::vector<int> all;
    const std::size_t p = encoding.characteristic;
    all.reserve((encoding.rows + encoding.columns + encoding.slices) * p);
    for (std::size_t index = 0; index < encoding.rows; ++index) {
        for (int variable : encoding.left_group(term, index)) all.push_back(variable);
    }
    for (std::size_t index = 0; index < encoding.columns; ++index) {
        for (int variable : encoding.right_group(term, index)) all.push_back(variable);
    }
    for (std::size_t index = 0; index < encoding.slices; ++index) {
        for (int variable : encoding.output_group(term, index)) all.push_back(variable);
    }
    return all;
}

}  // namespace

std::vector<int> PrimeFieldEncoding::left_group(std::size_t term, std::size_t row) const {
    return slice_of(left, term * rows + row, characteristic);
}
std::vector<int> PrimeFieldEncoding::right_group(std::size_t term, std::size_t column) const {
    return slice_of(right, term * columns + column, characteristic);
}
std::vector<int> PrimeFieldEncoding::output_group(std::size_t term, std::size_t slice) const {
    return slice_of(output, term * slices + slice, characteristic);
}

PrimeFieldEncoding encode_prime_rank_at_most(const linear_algebra::Tensor& tensor,
                                             std::size_t products, bool break_symmetry) {
    const std::size_t characteristic = static_cast<std::size_t>(tensor.characteristic);
    if (!linear_algebra::is_prime(tensor.characteristic)) {
        throw std::invalid_argument("GF(" + std::to_string(tensor.characteristic) +
                                    ") is not a prime field, and this writes out a prime's tables");
    }
    if (characteristic > 32) {
        throw std::invalid_argument("a table for GF(" + std::to_string(characteristic) +
                                    ") is " + std::to_string(characteristic * characteristic) +
                                    " clauses per operation; that is not the way to do it");
    }
    if (tensor.slices.empty()) throw std::invalid_argument("a tensor with no slices has rank 0");

    PrimeFieldEncoding encoding;
    encoding.characteristic = characteristic;
    encoding.products = products;
    encoding.rows = tensor.rows();
    encoding.columns = tensor.columns();
    encoding.slices = tensor.slices.size();

    const std::size_t entries = encoding.rows * encoding.columns * encoding.slices;
    if (products != 0 && entries > variable_budget() / (products * characteristic)) {
        throw std::invalid_argument("this encoding wants more than the budget of " +
                                    std::to_string(variable_budget()) + " variables");
    }

    linear_algebra::Cnf& formula = encoding.formula;
    const auto times = [characteristic](std::size_t a, std::size_t b) {
        return (a * b) % characteristic;
    };
    const auto plus = [characteristic](std::size_t a, std::size_t b) {
        return (a + b) % characteristic;
    };

    for (std::size_t term = 0; term < products; ++term) {
        for (std::size_t index = 0; index < encoding.rows; ++index) {
            for (int variable : one_hot(formula, characteristic)) encoding.left.push_back(variable);
        }
        for (std::size_t index = 0; index < encoding.columns; ++index) {
            for (int variable : one_hot(formula, characteristic)) encoding.right.push_back(variable);
        }
        for (std::size_t index = 0; index < encoding.slices; ++index) {
            for (int variable : one_hot(formula, characteristic)) encoding.output.push_back(variable);
        }
    }

    // q[l][i][j] = a[l][i] * b[l][j], shared across slices as in the GF(2) case.
    std::vector<std::vector<int>> pair(products * encoding.rows * encoding.columns);
    for (std::size_t term = 0; term < products; ++term) {
        for (std::size_t row = 0; row < encoding.rows; ++row) {
            for (std::size_t column = 0; column < encoding.columns; ++column) {
                pair[(term * encoding.rows + row) * encoding.columns + column] =
                    combined(formula, characteristic,
                             slice_of(encoding.left, term * encoding.rows + row, characteristic),
                             slice_of(encoding.right, term * encoding.columns + column,
                                      characteristic),
                             times);
            }
        }
    }

    for (std::size_t row = 0; row < encoding.rows; ++row) {
        for (std::size_t column = 0; column < encoding.columns; ++column) {
            for (std::size_t slice = 0; slice < encoding.slices; ++slice) {
                std::vector<int> running = constant_zero(formula, characteristic);
                for (std::size_t term = 0; term < products; ++term) {
                    const std::vector<int> contribution = combined(
                        formula, characteristic,
                        pair[(term * encoding.rows + row) * encoding.columns + column],
                        slice_of(encoding.output, term * encoding.slices + slice, characteristic),
                        times);
                    running = combined(formula, characteristic, running, contribution, plus);
                }

                const auto wanted = static_cast<std::size_t>(
                    (tensor.slices[slice](row, column) % tensor.characteristic +
                     tensor.characteristic) %
                    tensor.characteristic);
                formula.add_clause({running[wanted]});
            }
        }
    }

    if (break_symmetry) {
        for (std::size_t term = 0; term < products; ++term) {
            std::vector<std::vector<int>> left_vector;
            for (std::size_t row = 0; row < encoding.rows; ++row) {
                left_vector.push_back(encoding.left_group(term, row));
            }
            std::vector<std::vector<int>> right_vector;
            for (std::size_t column = 0; column < encoding.columns; ++column) {
                right_vector.push_back(encoding.right_group(term, column));
            }
            // Two of the three vectors, never all three: the output vector is
            // what absorbs the scalars the other two gave up.
            normalise_first_nonzero(formula, left_vector);
            normalise_first_nonzero(formula, right_vector);
        }
        for (std::size_t term = 0; term + 1 < products; ++term) {
            order_lexicographically(formula, term_variables(encoding, term),
                                    term_variables(encoding, term + 1));
        }
    }
    return encoding;
}

}  // namespace satisfiability
