// Is a candidate parent's kept pool content the list the pool scan would return?
//
// `candidate_parents` decides whether a hyperplane is reachable by testing the
// child's pool elements for membership, and now keeps the indices that passed. The
// parent test used to throw them away and ask `pool_inside`, which walks every
// rank-one map of the shape: 261 121 of them at `⟨3,3,3⟩` against the fifteen
// membership tests already done.
//
// The two are equal because `span(parent) ⊆ span(child)`, so no pool element
// outside the child's content can be inside a parent's. That is a claim about
// every parent the search visits, so it is checked on every parent the search
// visits, list against list rather than length against length.
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "canonical_parent.h"
#include "check.h"
#include "group_construction.h"
#include "pool_set_canon.h"
#include "search_children.h"
#include "span_basis.h"
#include "tensor_file.h"

namespace {

struct Tally {
    long long parents = 0;
    long long mismatches = 0;
};

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";
    const formats::Tensor tensor =
        formats::read_tensor_file(directory + "/matmul_2x2x2.tensor");
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
    const bilinear_rank::PoolSetCanon canon(
        field, bilinear_rank::matrix_multiplication_symmetry_generators(field, 2, 2, 2),
        tensor.rows(), tensor.columns());

    Tally tally;
    const std::size_t base = linear_algebra::span_of(field, tensor.slices).dimension();
    search_children::below(
        field, tensor.slices, pool, canon, tensor.slices, base, 7,
        [&](const std::vector<bilinear_rank::Matrix>& child) {
            const std::vector<std::size_t> inside = bilinear_rank::pool_inside(field, pool, child);
            const std::vector<bilinear_rank::Matrix> chosen(
                child.begin() + static_cast<std::ptrdiff_t>(tensor.slices.size()), child.end());
            for (const bilinear_rank::CandidateParent& parent :
                 bilinear_rank::candidate_parents(field, tensor.slices, chosen, pool, inside)) {
                ++tally.parents;
                if (parent.content != bilinear_rank::pool_inside(field, pool, parent.generators)) {
                    ++tally.mismatches;
                }
            }
        });

    std::cout << "  <2,2,2>: " << tally.parents << " candidate parents compared\n";
    check::equal("enough parents to be a sample", tally.parents > 100 ? 1 : 0, 1);
    check::equal("parents whose kept content differs from the pool scan", tally.mismatches, 0);

    return check::report("a candidate parent's pool content, kept against scanned");
}
