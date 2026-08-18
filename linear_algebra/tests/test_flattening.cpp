/// The flattening lower bound, checked in the direction that can be wrong.
///
/// A bound that is too low is merely weak. A bound that is too high is a false
/// lower bound, and nothing downstream catches one: `decide-rank` refuses every
/// target beneath it without searching, so the refusal looks like a proof. So the
/// checks here are mostly tensors whose rank is known from above by
/// construction, where the bound has to stay under a number we already hold.
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "check.h"
#include "linear_algebra.h"
#include "tensor_file.h"

namespace {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

/// Slices `e_i e_iᵀ`, whose rank is exactly `size`: this is the case where the
/// bound is tight, so it pins the arithmetic as well as the inequality.
std::vector<ModularMatrix> diagonal_tensor(std::size_t size) {
    std::vector<ModularMatrix> slices;
    for (std::size_t index = 0; index < size; ++index) {
        ModularMatrix slice(size, size);
        slice(index, index) = 1;
        slices.push_back(slice);
    }
    return slices;
}

/// A sum of `terms` rank-one terms, so the rank is at most `terms` however the
/// coefficients fall. The bound must not exceed it.
std::vector<ModularMatrix> low_rank_tensor(const ModularField& field, int64_t characteristic,
                                           std::mt19937& generator, std::size_t rows,
                                           std::size_t columns, std::size_t depth,
                                           std::size_t terms) {
    std::vector<ModularMatrix> slices(depth, ModularMatrix(rows, columns));
    std::uniform_int_distribution<int64_t> entries(0, characteristic - 1);
    for (std::size_t term = 0; term < terms; ++term) {
        std::vector<int64_t> left(rows), right(columns), coefficient(depth);
        for (int64_t& entry : left) entry = entries(generator);
        for (int64_t& entry : right) entry = entries(generator);
        for (int64_t& entry : coefficient) entry = entries(generator);
        for (std::size_t slice = 0; slice < depth; ++slice) {
            for (std::size_t row = 0; row < rows; ++row) {
                for (std::size_t column = 0; column < columns; ++column) {
                    int64_t product = 0;
                    field.mul(product, left[row], right[column]);
                    field.mulin(product, coefficient[slice]);
                    field.addin(slices[slice](row, column), product);
                }
            }
        }
    }
    return slices;
}

/// The claim `fewest_products.cpp` makes in a comment, checked rather than
/// asserted: flattening along the slice axis is the span of the slices.
void check_third_flattening_is_the_span(const ModularField& field,
                                       const std::vector<ModularMatrix>& slices,
                                       const std::string& what) {
    const std::size_t along_slices =
        linear_algebra::rank(field, linear_algebra::flattening(field, slices, 2));
    check::equal(what + ": slice-axis flattening rank is the span dimension",
                 static_cast<long long>(along_slices),
                 static_cast<long long>(linear_algebra::span_of(field, slices).dimension()));
}

void check_bound_is_under(const ModularField& field, const std::vector<ModularMatrix>& slices,
                          std::size_t known_from_above, const std::string& what) {
    const std::size_t bound = linear_algebra::flattening_lower_bound(field, slices);
    if (bound <= known_from_above) return;
    std::cout << "  FAIL  " << what << ": bound " << bound << " above a rank of at most "
              << known_from_above << "\n";
    ++check::failure_count;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_flattening <fixtures-directory>\n";
        return 2;
    }
    const std::string directory = argv[1];

    // Tight, and the only place an exact number is pinned here: the fixtures'
    // bounds live in the pipeline test, next to the counts they are compared to.
    for (int64_t characteristic : {2, 3, 5}) {
        const ModularField field(characteristic);
        for (std::size_t size = 1; size <= 5; ++size) {
            const std::vector<ModularMatrix> slices = diagonal_tensor(size);
            check::equal("GF(" + std::to_string(characteristic) + ") diagonal " +
                             std::to_string(size) + " bound",
                         static_cast<long long>(
                             linear_algebra::flattening_lower_bound(field, slices)),
                         static_cast<long long>(size));
            check::equal("GF(" + std::to_string(characteristic) + ") diagonal " +
                             std::to_string(size) + " concise",
                         linear_algebra::is_concise(field, slices), 1);
        }
    }

    // A 2x2 slice of rank one lives in a 1x1 space on every axis, so nothing here
    // is at full rank and the tensor is smaller than its shape.
    {
        const ModularField field(2);
        std::vector<ModularMatrix> slices(1, ModularMatrix(2, 2));
        for (std::size_t entry = 0; entry < 4; ++entry) slices[0].data()[entry] = 1;
        check::equal("rank-one slice bound",
                     static_cast<long long>(linear_algebra::flattening_lower_bound(field, slices)),
                     1);
        check::equal("rank-one slice is not concise", linear_algebra::is_concise(field, slices), 0);
    }

    // The direction that matters: never above a rank we already hold.
    std::mt19937 generator(20260816);
    for (int64_t characteristic : {2, 3, 5, 7}) {
        const ModularField field(characteristic);
        for (int trial = 0; trial < 30; ++trial) {
            const std::size_t rows = 1 + generator() % 5;
            const std::size_t columns = 1 + generator() % 5;
            const std::size_t depth = 1 + generator() % 5;
            const std::size_t terms = 1 + generator() % 8;
            const std::vector<ModularMatrix> slices = low_rank_tensor(
                field, characteristic, generator, rows, columns, depth, terms);
            const std::string what = "GF(" + std::to_string(characteristic) + ") trial " +
                                     std::to_string(trial);
            check_bound_is_under(field, slices, terms, what);
            check_third_flattening_is_the_span(field, slices, what);
        }
    }
    check::equal("random low-rank tensors bounded", check::failure_count, 0);

    // Every fixture: the naive algorithm is a decomposition somebody holds, so
    // its multiplication count is an upper bound the flattenings must respect.
    for (const char* name : {"f2_2x2", "f2_2x3", "f2_5x5", "f2_3x8", "f2_4x7", "f3_3x6",
                             "cyclic_f2_5", "gf4_multiplication", "gf8_multiplication",
                             "matmul_2x2x2", "matmul_2x2x3", "w_state"}) {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/" + std::string(name) + ".tensor");
        const ModularField field(tensor.characteristic);
        check_bound_is_under(field, tensor.slices,
                             linear_algebra::multiplication_count(field, tensor.slices), name);
        check_third_flattening_is_the_span(field, tensor.slices, name);

        // Flattening rearranges the entries and loses none of them.
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const ModularMatrix flat = linear_algebra::flattening(field, tensor.slices, axis);
            check::equal(std::string(name) + " axis " + std::to_string(axis) + " entry count",
                         static_cast<long long>(flat.entry_count()),
                         static_cast<long long>(tensor.slices.size() * tensor.rows() *
                                                tensor.columns()));
        }
    }

    return check::report("flattening bound");
}
