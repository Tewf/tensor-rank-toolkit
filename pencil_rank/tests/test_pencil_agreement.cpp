/// The pencil formula against the exhaustive search, on every two-slice tensor
/// in `fixtures/`.
///
/// This is how a new instrument earns trust here, and it has already been worth
/// it: the closure bound was shipped as an exact rank until this test refused
/// `pencil_irreducible_f2_4`, where the search proves 6 against the formula's 4.
/// The two share no code, one enumerating rank-one maps and walking a tree to
/// its end, the other diagonalising a polynomial matrix and counting factors.
///
/// So what is asserted is the claim actually made: the bound never exceeds the
/// rank, and where the pencil is diagonalisable over GF(p) it *is* the rank.
/// The searches below are seconds on 4x4 and hopeless by 8x8, which is why the
/// comparison has to be made while it still can be.
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "exhaustive_search.h"
#include "kronecker_structure.h"
#include "tensor_file.h"

namespace {

/// The fewest products the exhaustive search can find, swept upward from the
/// dimension of the slice space, which nothing can beat.
///
/// A spent budget is reported as 0 rather than as a rank, so the caller can
/// tell "no algorithm that small" from "did not finish". Silently returning the
/// next k up would turn giving up into an answer, which is the mistake
/// `infrastructure/cli/exit_code.h` exists to keep out of this repository.
long long fewest_products_by_search(const bilinear_rank::Field& field,
                                    const formats::Tensor& tensor, std::size_t ceiling) {
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    for (std::size_t target = 1; target <= ceiling; ++target) {
        bilinear_rank::SearchBudget budget{/*node_limit=*/20'000'000};
        std::vector<bilinear_rank::Matrix> products;
        if (bilinear_rank::expand_subspace(field, tensor.slices, pool, 0, target, budget,
                                           products)) {
            return static_cast<long long>(target);
        }
        if (!budget.tree_fully_walked) return 0;
    }
    return 0;
}

constexpr const char* kFixtures[] = {
    "gf4_multiplication", "w_state", "pencil_nilpotent_f2_3",
    "pencil_split_f3_3",  "pencil_singular_f2_2x3", "pencil_irreducible_f2_4",
};

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    for (const char* name : kFixtures) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + name + ".tensor");
        const bilinear_rank::Field field(tensor.characteristic);

        const pencil_rank::PencilRank reported = pencil_rank::pencil_rank_of(field, tensor.slices);
        // `proved`, not `over_closure`: since `[sumi2009, Thm. 3.5]` landed, the
        // module's best proved bound is the larger of the two and is the number
        // `decide-rank-by-pencil` prints. Reading the closure value here left
        // the test asserting that a bound the module no longer reports is the
        // rank, which is how it came to say 2 where the tool says 3.
        const long long bound = static_cast<long long>(reported.proved);

        // The ceiling is two above the bound, which covers every gap measured on
        // these shapes. A search allowed to climb without one would spend its
        // whole budget proving nothing on the fixture where the gap is largest.
        const long long by_search =
            fewest_products_by_search(field, tensor, static_cast<std::size_t>(bound) + 2);

        check::equal(std::string(name) + ": the search settled it", by_search > 0 ? 1 : 0, 1);
        check::equal(std::string(name) + ": the bound does not exceed the rank",
                     bound <= by_search ? 1 : 0, 1);
        if (reported.exact) {
            check::equal(std::string(name) + ": diagonalisable, so the bound is the rank",
                         bound, by_search);
        }
    }

    return check::report("pencil rank against exhaustion");
}
