#include <set>
#include <string>
#include <vector>

#include "automorphism.h"
#include "candidate_pool.h"
#include "canonical_augmentation.h"
#include "check.h"
#include "group_construction.h"
#include "tensor_file.h"

namespace {

/// A matrix written out entry by entry, so two of them can be compared as keys.
std::string key_of(const bilinear_rank::Matrix& matrix) {
    std::string key;
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            key += std::to_string(matrix(row, column)) + ",";
        }
    }
    return key;
}

std::string key_of(const bilinear_rank::Automorphism& sigma) {
    return key_of(sigma.left) + "|" + key_of(sigma.right);
}

}  // namespace

/// The one target that settles whether the deduplication is right.
///
/// `⟨2,2,2⟩` at 7 has 36 solution subspaces in a single orbit under its 216-element
/// group, computed twice by independent means and agreeing. So the plain enumerator
/// must find 36 and the canonical one must find 1, and any other pair means the
/// parent test is wrong rather than merely slow. The single orbit is de Groote's
/// uniqueness theorem for `⟨2,2,2⟩` recovered from the tensor.
int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";
    const linear_algebra::Tensor strassen =
        linear_algebra::read_tensor_file(directory + "/matmul_2x2x2.tensor");
    const bilinear_rank::Field field(strassen.characteristic);

    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, strassen.rows(), strassen.columns());
    const std::vector<bilinear_rank::Automorphism> group =
        bilinear_rank::matrix_multiplication_symmetries(field, 2, 2, 2);
    check::equal("pool of <2,2,2>", pool.size(), 225);
    check::equal("group of <2,2,2>", group.size(), 216);

    // group_construction.h says its two routes "check each other". Nothing
    // checked them: the brute-force route refuses on every matmul shape, the
    // smallest of which is this one, so the comparison can only be made the
    // other way round, by closing the generators the closed form also exposes.
    const std::vector<bilinear_rank::Automorphism> generated = bilinear_rank::group_closure(
        field, bilinear_rank::matrix_multiplication_symmetry_generators(field, 2, 2, 2));
    check::equal("closing the generators gives the same order", generated.size(), group.size());

    std::set<std::string> listed;
    for (const bilinear_rank::Automorphism& sigma : group) listed.insert(key_of(sigma));
    std::set<std::string> closed;
    for (const bilinear_rank::Automorphism& sigma : generated) closed.insert(key_of(sigma));
    check::equal("and the same 216 elements, not merely as many", closed == listed, 1);

    const bilinear_rank::EnumerationReport plain =
        bilinear_rank::enumerate_solution_subspaces(field, strassen, pool, group, 7, false);
    check::equal("plain enumeration finds every solution subspace", plain.distinct, 36);
    check::equal("reaching each of them once per basis of the quotient", plain.emitted, 720);

    // The locus: how much of the pool the 36 solutions between them use. The
    // number was stated in canonical_augmentation.h and asserted nowhere, which
    // for a figure said to have been "computed twice and agreeing" is the one
    // state it should not have been left in.
    std::set<std::string> used;
    for (const std::vector<bilinear_rank::Matrix>& basis : plain.decompositions) {
        for (const bilinear_rank::Matrix& term : basis) used.insert(key_of(term));
    }
    check::equal("the solutions between them use 90 of the 225 rank-one maps", used.size(), 90);

    const bilinear_rank::EnumerationReport canonical =
        bilinear_rank::enumerate_solution_subspaces(field, strassen, pool, group, 7, true);
    check::equal("canonical augmentation finds one per orbit", canonical.distinct, 1);
    check::equal("and reaches it exactly once", canonical.emitted, 1);

    // Every emitted basis was multiplied out against the map inside the enumerator,
    // so reaching here means all of them compute <2,2,2>. Its size is the other half.
    check::equal("and it is a seven-product algorithm", canonical.decompositions.front().size(), 7);
    return check::report("canonical_augmentation");
}
