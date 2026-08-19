#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "automorphism.h"
#include "candidate_pool.h"
#include "canonical_augmentation.h"
#include "check.h"
#include "group_construction.h"
#include "parallel.h"
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

/// Everything an enumeration reports that does not depend on the machine.
struct Counts {
    long long emitted = 0;
    long long distinct = 0;
    long long nodes = 0;
    long long group_visits = 0;
};

Counts counts_of(const bilinear_rank::EnumerationReport& pass) {
    return {static_cast<long long>(pass.emitted), static_cast<long long>(pass.distinct),
            static_cast<long long>(pass.nodes), static_cast<long long>(pass.group_visits)};
}

/// One pass at `workers` workers, leaving the count back at one afterwards so a
/// later check cannot inherit it.
Counts enumerate_with(std::size_t workers, const bilinear_rank::Field& field,
                      const linear_algebra::Tensor& tensor,
                      const std::vector<bilinear_rank::Matrix>& pool,
                      const std::vector<bilinear_rank::Automorphism>& group, std::size_t target,
                      bool canonical) {
    bilinear_rank::set_worker_count(workers);
    const Counts counts = counts_of(
        bilinear_rank::enumerate_solution_subspaces(field, tensor, pool, group, target, canonical));
    bilinear_rank::set_worker_count(1);
    return counts;
}

void same_counts(const std::string& what, const Counts& actual, const Counts& expected) {
    check::equal(what + ", paths", actual.emitted, expected.emitted);
    check::equal(what + ", distinct subspaces", actual.distinct, expected.distinct);
    check::equal(what + ", nodes", actual.nodes, expected.nodes);
    check::equal(what + ", group visits", actual.group_visits, expected.group_visits);
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

    // The node totals the deduplication exists to move. Quoted in
    // `CMakeLists.txt` and in the module's prose and asserted nowhere, which for
    // the pair of numbers that says what canonical augmentation bought is the one
    // state they should not have been left in.
    check::equal("the plain route pays 1 890 601 nodes for those 36", plain.nodes, 1890601);
    check::equal("and the canonical route 954 for the 1", canonical.nodes, 954);

    // Threads change no count here, and that is a claim about this enumeration
    // rather than about searches in general: it counts instead of stopping, so
    // there is no witness to race to and no shared budget to spend in a subtree
    // that will not win. The exact search has both, and there a thread count is
    // visible in a node total and can be visible in a verdict; see
    // `exhaustive_search/what-threads-change.md`. So the assertion below is the
    // one that separates the two cases, and it is made rather than assumed.
    //
    // At 6 the question is a refutation, 25 399 plain nodes and 103 canonical
    // ones, which is cheap enough to ask at every thread count worth asking at.
    for (const bool route : {false, true}) {
        const std::string name = route ? "canonical at 6" : "plain at 6";
        const Counts one = enumerate_with(1, field, strassen, pool, group, 6, route);
        for (const std::size_t workers : {std::size_t(2), std::size_t(4), std::size_t(6),
                                          std::size_t(12)}) {
            same_counts(name + " on " + std::to_string(workers) + " workers",
                        enumerate_with(workers, field, strassen, pool, group, 6, route), one);
        }
    }

    // And at 7, where there are solutions to find, so that the merge across
    // branches is exercised: a subspace reachable down two of them must still be
    // counted once. Six workers rather than twelve because 6 is the thread count
    // this chassis reproduces (`MEASURING.md`), and one route each is enough
    // once the sweep above has covered the thread counts.
    same_counts("canonical at 7 on 6 workers",
                enumerate_with(6, field, strassen, pool, group, 7, true), counts_of(canonical));
    same_counts("plain at 7 on 6 workers",
                enumerate_with(6, field, strassen, pool, group, 7, false), counts_of(plain));

    return check::report("canonical_augmentation");
}
