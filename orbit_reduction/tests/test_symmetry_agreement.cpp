#include <stdexcept>
/// That quotienting a search by the map's symmetries changes what it costs and
/// never what it answers.
///
/// This is the only test that can catch a wrong symmetry group, and a wrong
/// symmetry group is the one mistake in this repository that nothing downstream
/// catches: it does not crash, it does not return a decomposition that fails
/// `recovers_map`, it simply turns a question that has an answer into one that
/// appears not to, and the result is a lower bound nobody can see is false.
///
/// So every fixture is asked twice, once trying every candidate and once trying
/// one per orbit, at a `k` that is satisfiable and at a `k` that is not, and the
/// two runs must agree on both.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "automorphism.h"
#include "candidate_pool.h"
#include "check.h"
#include "exhaustive_search.h"
#include "group_construction.h"
#include "orbit_search.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

/// What a search concluded, keeping "refuted" and "ran out of budget" apart.
/// Collapsing those two is how a budget becomes a proof.
enum class Verdict { Found, Refuted, Undecided };

const char* name_of(Verdict verdict) {
    switch (verdict) {
        case Verdict::Found: return "found";
        case Verdict::Refuted: return "refuted";
        case Verdict::Undecided: return "undecided";
    }
    return "?";
}

Verdict plain(const Field& field, const std::vector<Matrix>& slices,
              const std::vector<Matrix>& pool, std::size_t target) {
    bilinear_rank::SearchBudget budget{2'000'000};
    std::vector<Matrix> products;
    if (bilinear_rank::expand_subspace(field, slices, pool, 0, target, budget, products)) {
        return Verdict::Found;
    }
    return budget.exhausted ? Verdict::Refuted : Verdict::Undecided;
}

Verdict quotiented(const Field& field, const std::vector<Matrix>& slices,
                   const std::vector<Matrix>& pool, std::size_t target) {
    const std::vector<bilinear_rank::Automorphism> group = bilinear_rank::stabiliser_of(
        field, slices,
        bilinear_rank::all_automorphisms(field, slices.front().rows(), slices.front().columns()));

    bilinear_rank::SearchBudget budget{2'000'000};
    std::vector<Matrix> products;
    if (bilinear_rank::expand_subspace_up_to_symmetry(field, slices, pool, group, target, budget, products)) {
        return Verdict::Found;
    }
    return budget.exhausted ? Verdict::Refuted : Verdict::Undecided;
}

struct Question {
    const char* fixture;
    std::size_t target;
    Verdict expected;
};

/// Small enough that the ambient group can actually be built: a 2x2 map over
/// GF(2) needs 36 automorphism pairs and a 2x3 needs 1008. The 4x4 of
/// `⟨2,2,2⟩` would need four hundred million, which is what `--symmetry matmul`
/// exists for.
constexpr Question kQuestions[] = {
    {"f2_2x2", 3, Verdict::Found},   {"f2_2x2", 2, Verdict::Refuted},
    {"f2_2x3", 5, Verdict::Found},   {"f2_2x3", 4, Verdict::Refuted},
};

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_symmetry_agreement <fixtures>\n";
        return 1;
    }
    const std::string directory = argv[1];

    for (const Question& question : kQuestions) {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/" + question.fixture + ".tensor");
        const Field field(tensor.characteristic);
        const std::vector<Matrix> pool =
            bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

        const std::string label =
            std::string(question.fixture) + " at k = " + std::to_string(question.target);
        const Verdict without = plain(field, tensor.slices, pool, question.target);
        const Verdict with = quotiented(field, tensor.slices, pool, question.target);

        check::equal(label + ", without orbits", static_cast<long long>(without),
                     static_cast<long long>(question.expected));
        if (with != without) {
            std::cout << "  FAIL  " << label << ": orbits say " << name_of(with)
                      << " and no orbits say " << name_of(without)
                      << ", so the group is wrong\n";
            ++check::failure_count;
        } else {
            std::cout << "  ok    " << label << ": orbits agree (" << name_of(with) << ")\n";
        }
    }
    // A named shape that is not the tensor's shape must be refused, not used.
    //
    // Before the guard, `matmul 2 2 2` against 4x6 slices reached `act_on`, which
    // multiplies matrices whose sizes do not meet through a `multiply` that checks
    // none, and the write ran past the end of the result: `free(): invalid
    // pointer` and exit 134 on the tree refuter. On `minimise-rank` it was quieter
    // and worse, exit 0 having quotiented by nothing at all. The mismatch is
    // therefore a memory-safety property and not a usability one, which is why it
    // is asserted rather than left to the tools.
    {
        const linear_algebra::Tensor wrong =
            linear_algebra::read_tensor_file(directory + "/matmul_2x2x3.tensor");
        bool refused = false;
        try {
            bilinear_rank::require_matmul_shape(wrong.slices, 2, 2, 2);
        } catch (const std::runtime_error&) {
            refused = true;
        }
        check::equal("a shape that is not the tensor's is refused", refused ? 1 : 0, 1);

        const linear_algebra::Tensor right =
            linear_algebra::read_tensor_file(directory + "/matmul_2x2x2.tensor");
        bool accepted = true;
        try {
            bilinear_rank::require_matmul_shape(right.slices, 2, 2, 2);
        } catch (const std::runtime_error&) {
            accepted = false;
        }
        check::equal("and the tensor's own shape is not", accepted ? 1 : 0, 1);

        // 2x2x3 against its own shape, so the guard is not merely refusing
        // everything that is not 2x2x2.
        bool own = true;
        try {
            bilinear_rank::require_matmul_shape(wrong.slices, 2, 2, 3);
        } catch (const std::runtime_error&) {
            own = false;
        }
        check::equal("nor is 2x2x3 against 2 2 3", own ? 1 : 0, 1);
    }

    return check::report("symmetry agreement");
}
