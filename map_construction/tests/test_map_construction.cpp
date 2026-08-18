/// Building maps, and deciding two whose answers are classical.
///
/// The bilinear complexity of multiplication in GF(4) over GF(2) is 3, and in
/// GF(8) it is 6. Both are known independently of anything here, so they check
/// the construction and the exact search at once.
#include <array>
#include <iostream>
#include <random>
#include <string>

#include "candidate_pool.h"
#include "check.h"
#include "exhaustive_search.h"
#include "fewest_products.h"
#include "map_construction.h"

namespace {

void check_irreducibility(const bilinear_rank::Field& field) {
    // Over GF(2): x^2+x+1 and x^3+x+1 are irreducible, x^2+1 = (x+1)^2 is not.
    check::equal("x^2+x+1 irreducible over F2",
                 bilinear_rank::is_irreducible(field, {1, 1, 1}) ? 1 : 0, 1);
    check::equal("x^2+1 reducible over F2",
                 bilinear_rank::is_irreducible(field, {1, 0, 1}) ? 1 : 0, 0);
    check::equal("x^3+x+1 irreducible over F2",
                 bilinear_rank::is_irreducible(field, {1, 0, 1, 1}) ? 1 : 0, 1);
}

void check_polynomial_tensor() {
    const std::vector<bilinear_rank::Matrix> tensor =
        bilinear_rank::polynomial_multiplication_tensor(5, 5);
    check::equal("5x5 polynomial tensor slices", static_cast<long long>(tensor.size()), 9);
    const bilinear_rank::Field field(2);
    check::equal("5x5 polynomial tensor naive cost",
                 static_cast<long long>(linear_algebra::multiplication_count(field, tensor)), 25);
}

/// GF(4) multiplication: c1 = a0b1 + a1b0 + a1b1, c0 = a0b0 + a1b1, because
/// x^2 = x + 1.
void check_gf4_tensor(const bilinear_rank::Field& field) {
    const std::vector<bilinear_rank::Matrix> tensor =
        bilinear_rank::field_multiplication_tensor(field, {1, 1, 1});
    check::equal("GF(4) tensor slices", static_cast<long long>(tensor.size()), 2);
    if (tensor.size() != 2) return;

    const int64_t expected_x[2][2] = {{0, 1}, {1, 1}};
    const int64_t expected_1[2][2] = {{1, 0}, {0, 1}};
    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t column = 0; column < 2; ++column) {
            check::equal("GF(4) x coefficient", tensor[0](row, column), expected_x[row][column]);
            check::equal("GF(4) constant coefficient", tensor[1](row, column),
                         expected_1[row][column]);
        }
    }
    check::equal("GF(4) naive cost",
                 static_cast<long long>(linear_algebra::multiplication_count(field, tensor)), 4);
}

/// The exact search must reach the classical value from nothing.
void check_field_complexity(const bilinear_rank::Field& field,
                            const bilinear_rank::Polynomial& modulus, const std::string& what,
                            long long expected) {
    const std::vector<bilinear_rank::Matrix> tensor =
        bilinear_rank::field_multiplication_tensor(field, modulus);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.front().rows(), tensor.front().columns());

    bilinear_rank::SearchBudget budget;
    std::vector<bilinear_rank::Matrix> products;
    if (!bilinear_rank::fewest_products_by_sweep(field, tensor, pool, budget, products)) {
        std::cout << "  FAIL  " << what << ": no decomposition found\n";
        ++check::failure_count;
        return;
    }
    check::equal(what + " bilinear complexity", static_cast<long long>(products.size()), expected);
}

/// The check that matters for a new constructor: does the tensor actually
/// compute the product it claims?
///
/// Evaluating slice `s` at (vec A, vec B) must give C[s] for random A and B, in
/// every shape. A tensor that is plausible but transposed somewhere would pass
/// a slice count and a naive cost, and then quietly decompose the wrong map.
void check_matmul_computes_the_product(const bilinear_rank::Field& field) {
    std::mt19937 generator(26041981);
    for (const std::array<std::size_t, 3> shape :
         {std::array<std::size_t, 3>{2, 2, 2}, {2, 2, 3}, {2, 3, 3}, {3, 3, 3}, {1, 4, 2}}) {
        const std::size_t rows = shape[0], inner = shape[1], columns = shape[2];
        const std::vector<bilinear_rank::Matrix> tensor =
            bilinear_rank::matrix_multiplication_tensor(rows, inner, columns);

        const std::string what = "<" + std::to_string(rows) + "," + std::to_string(inner) + "," +
                                 std::to_string(columns) + ">";
        check::equal(what + " slice count", static_cast<long long>(tensor.size()),
                     static_cast<long long>(rows * columns));
        check::equal(what + " naive cost",
                     static_cast<long long>(linear_algebra::multiplication_count(field, tensor)),
                     static_cast<long long>(rows * inner * columns));

        for (int trial = 0; trial < 8; ++trial) {
            std::vector<int64_t> left(rows * inner), right(inner * columns);
            for (int64_t& entry : left) entry = static_cast<int64_t>(generator() % 2);
            for (int64_t& entry : right) entry = static_cast<int64_t>(generator() % 2);

            for (std::size_t row = 0; row < rows; ++row) {
                for (std::size_t column = 0; column < columns; ++column) {
                    // What the product is, by definition.
                    int64_t expected = 0;
                    for (std::size_t shared = 0; shared < inner; ++shared) {
                        expected ^= left[row * inner + shared] & right[shared * columns + column];
                    }
                    // What the slice evaluates to: (vec A)^T S (vec B).
                    const bilinear_rank::Matrix& slice = tensor[row * columns + column];
                    int64_t got = 0;
                    for (std::size_t index = 0; index < left.size(); ++index) {
                        for (std::size_t other = 0; other < right.size(); ++other) {
                            got ^= left[index] & right[other] & slice(index, other);
                        }
                    }
                    if (got != expected) {
                        std::cout << "  FAIL  " << what << " entry (" << row << "," << column
                                  << ") computes " << got << ", the product is " << expected
                                  << "\n";
                        ++check::failure_count;
                    }
                }
            }
        }
    }
}

/// Cyclic convolution: every slice is a permutation matrix, so the naive cost
/// is n^2 and each slice has full rank n.
void check_cyclic_tensor(const bilinear_rank::Field& field) {
    for (std::size_t length : {3, 4, 5}) {
        const std::vector<bilinear_rank::Matrix> tensor =
            bilinear_rank::cyclic_convolution_tensor(length);
        const std::string what = "cyclic " + std::to_string(length);
        check::equal(what + " slices", static_cast<long long>(tensor.size()),
                     static_cast<long long>(length));
        check::equal(what + " naive cost",
                     static_cast<long long>(linear_algebra::multiplication_count(field, tensor)),
                     static_cast<long long>(length * length));
    }
}

}  // namespace

int main() {
    const bilinear_rank::Field over_two(2);

    check_irreducibility(over_two);
    check_polynomial_tensor();
    check_matmul_computes_the_product(over_two);
    check_cyclic_tensor(over_two);
    check_gf4_tensor(over_two);
    check_field_complexity(over_two, {1, 1, 1}, "GF(4) over GF(2)", 3);
    check_field_complexity(over_two, {1, 1, 0, 1}, "GF(8) over GF(2)", 6);

    return check::report("map construction");
}
