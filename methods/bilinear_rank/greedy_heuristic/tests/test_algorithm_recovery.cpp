/// The round trip that matters: a decomposition must turn back into an
/// algorithm that computes the map it came from.
#include <iostream>
#include <random>
#include <string>

#include "algorithm_recovery.h"
#include "candidate_pool.h"
#include "check.h"
#include "minimum_weight_basis.h"
#include "tensor_file.h"

namespace {

bool same_map(const bilinear_rank::Field& field, const std::vector<bilinear_rank::Matrix>& left,
              const std::vector<bilinear_rank::Matrix>& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t slice = 0; slice < left.size(); ++slice) {
        if (left[slice].entry_count() != right[slice].entry_count()) return false;
        for (std::size_t entry = 0; entry < left[slice].entry_count(); ++entry) {
            // Compared in the field, so two representatives of the same residue
            // never read as different maps.
            if (!field.areEqual(left[slice].data()[entry], right[slice].data()[entry])) {
                return false;
            }
        }
    }
    return true;
}

/// map -> decomposition -> (L, R, P) -> map, and the last must equal the first.
void check_round_trip(const bilinear_rank::Field& field,
                      const std::vector<bilinear_rank::Matrix>& map, const std::string& what,
                      long long expected_products = -1) {
    const std::vector<bilinear_rank::Matrix> basis = bilinear_rank::minimum_weight_basis(field, map);
    const std::vector<bilinear_rank::Matrix> products =
        bilinear_rank::rank_one_candidates(field, basis);

    bilinear_rank::Algorithm algorithm;
    if (!bilinear_rank::recover_algorithm(field, map, products, algorithm)) {
        std::cout << "  FAIL  " << what << ": recovery failed\n";
        ++check::failure_count;
        return;
    }

    // One product per multiplication the search charges for.
    check::equal(what + " products",
                 static_cast<long long>(algorithm.product_count()),
                 static_cast<long long>(linear_algebra::multiplication_count(field, basis)));
    if (expected_products >= 0) {
        check::equal(what + " products against the published cost",
                     static_cast<long long>(algorithm.product_count()), expected_products);
    }

    if (!same_map(field, bilinear_rank::map_computed_by(field, algorithm), map)) {
        std::cout << "  FAIL  " << what << ": the recovered algorithm computes a different map\n";
        ++check::failure_count;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_algorithm_recovery <fixtures-directory>\n";
        return 2;
    }
    const std::string directory = argv[1];

    // Step 1's cost on each fixture, from results.json.
    const std::pair<const char*, long long> fixtures[] = {
        {"f2_5x5", 16}, {"f2_3x8", 19}, {"f2_4x7", 19}, {"f3_3x6", 12}};

    for (const auto& [name, products] : fixtures) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + std::string(name) + ".tensor");
        const bilinear_rank::Field field(tensor.characteristic);
        check_round_trip(field, tensor.slices, name, products);
    }

    // This test exercises rank-one slices with a zero first row, a case that
    // structured fixtures miss.
    std::mt19937 generator(26041981);
    for (int64_t characteristic : {2, 3, 5}) {
        const bilinear_rank::Field field(characteristic);
        std::uniform_int_distribution<int64_t> entries(0, characteristic - 1);
        for (int trial = 0; trial < 12; ++trial) {
            const std::size_t rows = 2 + generator() % 3;
            const std::size_t columns = 2 + generator() % 3;
            std::vector<bilinear_rank::Matrix> map;
            for (std::size_t slice = 0; slice < 3; ++slice) {
                bilinear_rank::Matrix matrix(rows, columns);
                // Force a zero first row half the time.
                const bool blank_first_row = (trial % 2 == 0);
                for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
                    matrix.data()[entry] = entries(generator);
                }
                if (blank_first_row) {
                    for (std::size_t column = 0; column < columns; ++column) matrix(0, column) = 0;
                }
                map.push_back(std::move(matrix));
            }
            check_round_trip(field, map,
                             "random GF(" + std::to_string(characteristic) + ") trial " +
                                 std::to_string(trial));
        }
    }

    return check::report("algorithm recovery");
}
