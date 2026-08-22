/// Sparsification: how few nonzeros each method reaches, and that what it
/// returns is still the same operator.
#include <iostream>
#include <random>
#include <string>

#include "check.h"
#include "combinations.h"
#include "dense_matrix_file.h"
#include "greedy_sparsifier.h"
#include "linear_algebra.h"
#include "rational_sparsifier.h"

namespace {

struct Expectation {
    const char* name;
    long long as_given;
    long long sparsified;
    /// `nnz + nns`, the cost the article minimises.
    ///
    /// **This was 20 for the oracles and is 10 for the exact method**, on the
    /// alternative-basis operator whose entries are ninths. The oracles left all
    /// ten entries as ninths, twenty operations; the exact method returns a
    /// basis of ten signs, ten. It does not *aim* at this — it minimises zeros
    /// and breaks ties by the order it walks supports in — so read the 10 as a
    /// measurement of one tie-break and not as a guarantee. The method that
    /// guarantees it is the greedy by rescaling, in the column beside.
    long long exact_operations;
    long long greedy_operations;
};

constexpr Expectation kExpectations[] = {
    {"strassen_u", 12, 10, 10, 10},
    {"strassen_v", 12, 10, 10, 10},
    {"alternative_basis_u", 21, 10, 10, 10},
};

void check_equivalent(const matrix_sparsification::Field& field, const matrix_sparsification::Matrix& original,
                      const matrix_sparsification::Matrix& result, const std::string& what) {
    if (!linear_algebra::same_row_space(field, linear_algebra::transpose<matrix_sparsification::Field>(original),
                               linear_algebra::transpose<matrix_sparsification::Field>(result))) {
        std::cout << "  FAIL  " << what << ": not the same operator\n";
        ++check::failure_count;
    }
}

void check_combinations() {
    check::equal("combinations(7,4) count", static_cast<long long>(matrix_sparsification::combinations(7, 4).size()), 35);
    check::equal("combinations(4,4) count", static_cast<long long>(matrix_sparsification::combinations(4, 4).size()), 1);
    check::equal("combinations(4,5) count", static_cast<long long>(matrix_sparsification::combinations(4, 5).size()), 0);
    check::equal("combinations(5,0) count", static_cast<long long>(matrix_sparsification::combinations(5, 0).size()), 1);
}

/// The inverse has to be exact, not nearly right: it is what the whole
/// heuristic multiplies by.
void check_inverse_round_trip(const matrix_sparsification::Field& field) {
    std::mt19937 generator(26041981);
    std::uniform_int_distribution<int> numerators(-9, 9);
    std::uniform_int_distribution<int> denominators(1, 9);

    int inverted = 0;
    for (int trial = 0; trial < 60; ++trial) {
        const std::size_t order = 2 + generator() % 4;
        matrix_sparsification::Matrix matrix(order, order);
        for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
            matrix.data()[entry] =
                Givaro::Rational(Givaro::Integer(numerators(generator)),
                                 Givaro::Integer(denominators(generator)));
        }
        matrix_sparsification::Matrix inverse;
        if (!linear_algebra::invert(field, matrix, inverse)) continue;
        ++inverted;

        const matrix_sparsification::Matrix product = linear_algebra::multiply(field, matrix, inverse);
        for (std::size_t row = 0; row < order; ++row) {
            for (std::size_t column = 0; column < order; ++column) {
                const bool should_be_one = (row == column);
                const bool is_one = field.isOne(product(row, column));
                const bool is_zero = field.isZero(product(row, column));
                if (should_be_one ? !is_one : !is_zero) {
                    std::cout << "  FAIL  inverse round trip, trial " << trial << "\n";
                    ++check::failure_count;
                    return;
                }
            }
        }
    }
    check::equal("random rational matrices inverted", inverted > 40 ? 1 : 0, 1);
}

/// Shapes the oracles are never handed by the tool, pinned because they
/// terminate for a reason that is easy to break.
///
/// Both walk column subsets downward from `columns - 1`. With no columns at all
/// that start is `SIZE_MAX`, and what stops the loop is the second wrap in
/// `size + 1 >= rows()`, not the first. Anyone rewriting that condition into
/// something that reads more naturally can turn it into a loop of 2^64 rounds,
/// so the degenerate shapes are exercised rather than assumed.
void check_degenerate_shapes(const matrix_sparsification::Field& field) {
    for (const std::pair<std::size_t, std::size_t>& shape :
         {std::pair<std::size_t, std::size_t>{2, 0}, {1, 0}, {3, 2}, {4, 1}}) {
        matrix_sparsification::Matrix operand(shape.first, shape.second);
        for (std::size_t entry = 0; entry < operand.entry_count(); ++entry) {
            field.assign(operand.data()[entry], field.one);
        }
        const std::string what = std::to_string(shape.first) + "x" + std::to_string(shape.second);
        const matrix_sparsification::Matrix exact =
            matrix_sparsification::sparsest_basis_over_the_rationals(field, operand);
        const matrix_sparsification::Matrix greedy =
            matrix_sparsification::sparsify_by_rescaling(field, operand);
        check::equal(what + " greedy keeps the shape",
                     static_cast<long long>(greedy.entry_count()),
                     static_cast<long long>(operand.entry_count()));
        check::equal(what + " exact keeps the shape",
                     static_cast<long long>(exact.entry_count()),
                     static_cast<long long>(operand.entry_count()));
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_sparsify <fixtures-directory>\n";
        return 2;
    }
    const std::string directory = argv[1];
    const matrix_sparsification::Field field;

    check_combinations();
    check_inverse_round_trip(field);
    check_degenerate_shapes(field);

    for (const Expectation& expected : kExpectations) {
        const std::string name = expected.name;
        const matrix_sparsification::Matrix original =
            linear_algebra::read_rational_matrix_file(directory + "/" + name + ".matrix");
        const matrix_sparsification::Matrix transposed = linear_algebra::transpose<matrix_sparsification::Field>(original);

        check::equal(name + " as given",
                     static_cast<long long>(linear_algebra::nonzero_count(field, original)),
                     expected.as_given);

        const matrix_sparsification::Matrix exact =
            linear_algebra::transpose<matrix_sparsification::Field>(
                matrix_sparsification::sparsest_basis_over_the_rationals(field, transposed));
        check_equivalent(field, original, exact, name + " exact over Q");
        check::equal(name + " exact over Q",
                     static_cast<long long>(linear_algebra::nonzero_count(field, exact)),
                     expected.sparsified);
        check::equal(name + " exact over Q operations",
                     static_cast<long long>(linear_algebra::operation_count(field, exact)),
                     expected.exact_operations);

        const matrix_sparsification::Matrix greedy =
            linear_algebra::transpose<matrix_sparsification::Field>(
                matrix_sparsification::sparsify_by_rescaling(field, transposed));
        check_equivalent(field, original, greedy, name + " greedy");
        check::equal(name + " greedy",
                     static_cast<long long>(linear_algebra::nonzero_count(field, greedy)),
                     expected.sparsified);
        check::equal(name + " greedy operations",
                     static_cast<long long>(linear_algebra::operation_count(field, greedy)),
                     expected.greedy_operations);
    }

    return check::report("sparsification");
}
