/// The theorems of `correctness.md`, executed rather than argued.
///
/// A proof in a markdown file stops being worth anything the moment the code
/// drifts from it, and nothing here would have said so. These are the two
/// theorems that are checkable in finite time, and one of them tests the
/// riskiest line in the module.
///
/// Only the two fixtures whose ambient group can actually be built are used, for
/// the same reason `test_symmetry_agreement` uses them: `general_linear_group`
/// stops at 4x4 slices and every larger shape goes through the closed form.
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "group_construction.h"
#include "measures.h"
#include "minimise_rank.h"
#include "span_queries.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

/// A matrix written out entry by entry, so images can be compared as keys.
std::string key_of(const Field& field, const Matrix& matrix) {
    std::string key;
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            key += std::to_string(matrix(row, column)) + ",";
        }
    }
    (void)field;
    return key;
}

struct Fixture {
    const char* name;
    long long rank;
};

constexpr Fixture kFixtures[] = {{"f2_2x2", 3}, {"f2_2x3", 5}};

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    for (const Fixture& fixture : kFixtures) {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/" + fixture.name + ".tensor");
        const Field field(tensor.characteristic);
        const std::string label = fixture.name;

        const std::vector<Matrix> pool =
            bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

        // Run the descent to its fixed point, exactly as step 3 does.
        const std::vector<Matrix> start =
            bilinear_rank::descend_from_own_basis(field, tensor.slices);
        const std::vector<Matrix> shortlist =
            bilinear_rank::improving_candidates(field, start, pool);
        const std::vector<Matrix> settled =
            bilinear_rank::minimise_rank(field, start, shortlist);

        // Theorem 2, soundness. The invariant is span(S) contains span(T), so
        // whatever comes back still computes the map it came from.
        check::equal(label + ": T2, the result still generates the map",
                     linear_algebra::spans_all(field, settled, tensor.slices) ? 1 : 0, 1);
        check::equal(label + ": and reaches the rank",
                     static_cast<long long>(
                         linear_algebra::multiplication_count(field, settled)),
                     fixture.rank);

        // Theorem 5, and the sharp one. The loop stops when no *surviving*
        // candidate improves; Lemma 4 says everything it discarded provably
        // could not have. So rescanning the whole pool must find nothing, and
        // a bug in the pruning of `survivors_after` shows up right here rather
        // than as a quietly worse answer three fixtures later.
        const std::vector<Matrix> still_improving =
            bilinear_rank::improving_candidates(field, settled, pool);
        check::equal(label + ": T5, no candidate in the whole pool improves it",
                     static_cast<long long>(still_improving.size()), 0);

        // Theorem 6. Improving is invariant under any automorphism fixing the
        // span, so the improving set is a union of orbits. Checked on the
        // starting point rather than the fixed point, where by T5 it is empty
        // and the claim would be vacuous.
        const std::vector<bilinear_rank::Automorphism> ambient =
            bilinear_rank::all_automorphisms(field, tensor.rows(), tensor.columns());
        const std::vector<bilinear_rank::Automorphism> stabiliser =
            bilinear_rank::stabiliser_of(field, start, ambient);

        const std::vector<Matrix> improving =
            bilinear_rank::improving_candidates(field, start, pool);
        std::vector<std::string> keys;
        keys.reserve(improving.size());
        for (const Matrix& phi : improving) keys.push_back(key_of(field, phi));

        std::size_t escaped = 0;
        for (const bilinear_rank::Automorphism& sigma : stabiliser) {
            for (const Matrix& phi : improving) {
                const std::string image = key_of(field, bilinear_rank::act_on(field, sigma, phi));
                bool present = false;
                for (const std::string& seen : keys) {
                    if (seen == image) { present = true; break; }
                }
                if (!present) ++escaped;
            }
        }
        check::equal(label + ": T6, the improving set is closed under the stabiliser",
                     static_cast<long long>(escaped), 0);
        check::equal(label + ": and the stabiliser is not the trivial group",
                     stabiliser.size() > 1 ? 1 : 0, 1);
    }

    return check::report("descent guarantees");
}
