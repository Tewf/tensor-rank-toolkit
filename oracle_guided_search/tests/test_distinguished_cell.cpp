// Does asking one cell agree with asking all of them?
//
// The parent test used to canonise the marked pair of every pool element of the
// child and accept the added one when its key tied the least. It now canonises one
// pair, the added element's, and reads the answer off that key alone;
// `canonical_parent.h` argues why the two are the same question.
//
// The argument is checked here **cell by cell** rather than in aggregate: a
// predicate that agreed only about which cell wins would pass a check on the
// search's verdicts and still be wrong about every other cell, and the next set it
// met could be the one where that mattered.
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <random>
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
    long long cells = 0;
    long long disagreements = 0;
};

/// Kept apart by where the set came from, because the two answer different
/// questions. The visited sets say the shipped search would not change its mind;
/// the random ones say the equality is a fact about the group and not a
/// coincidence of the sets a span happens to produce. A predicate only the random
/// half catches out is wrong all the same, so one total would hide which half
/// spoke.
struct Sweep {
    Tally visited;
    Tally random;
};

/// The three dimensions of `⟨rows, inner, columns⟩`, which name the group. The
/// slice shape the pool is built from comes from the tensor instead.
struct Shape {
    std::size_t rows = 0;
    std::size_t inner = 0;
    std::size_t columns = 0;
};

/// The predicate as it stood before the collapse: canonise every cell's pair, keep
/// the least key, and a cell is distinguished when its key ties that least.
std::vector<bool> distinguished_by_loop(const bilinear_rank::PoolSetCanon& canon,
                                        const std::vector<std::size_t>& indices) {
    std::vector<std::vector<std::size_t>> keys;
    std::vector<std::size_t> best;
    for (const std::size_t index : indices) {
        keys.push_back(canon.canonical_with_marked(indices, index));
        if (best.empty() || keys.back() < best) best = keys.back();
    }
    std::vector<bool> distinguished;
    for (const std::vector<std::size_t>& key : keys) distinguished.push_back(key == best);
    return distinguished;
}

void compare_on(const bilinear_rank::PoolSetCanon& canon, const std::vector<std::size_t>& indices,
                Tally& tally) {
    if (indices.empty()) return;
    const std::vector<bool> loop = distinguished_by_loop(canon, indices);
    for (std::size_t cell = 0; cell < indices.size(); ++cell) {
        ++tally.cells;
        if (bilinear_rank::is_distinguished_cell(canon, indices, indices[cell]) != loop[cell]) {
            ++tally.disagreements;
        }
    }
}

/// A subset of the pool of one of the sizes the search's own sets have, drawn
/// without replacement and sorted, since that is the shape `pool_inside` returns.
std::vector<std::size_t> random_set(std::mt19937& source, std::size_t pool_size,
                                    std::size_t wanted) {
    std::vector<std::size_t> indices;
    for (std::size_t index = 0; index < pool_size; ++index) indices.push_back(index);
    std::shuffle(indices.begin(), indices.end(), source);
    indices.resize(wanted);
    std::sort(indices.begin(), indices.end());
    return indices;
}

/// Both predicates on every set the search hands the parent test down to
/// `target`, and on `random_sets` subsets of the pool besides.
///
/// The random ones are not spans of anything, so they are sets the search never
/// builds; the equality being checked is a fact about the group acting on subsets
/// and owes nothing to where the subset came from.
Sweep sweep(const std::string& file, const Shape& shape, std::size_t depth,
            std::size_t random_sets, std::mt19937& source) {
    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(file);
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
    const bilinear_rank::PoolSetCanon canon(
        field,
        bilinear_rank::matrix_multiplication_symmetry_generators(field, shape.rows, shape.inner,
                                                                 shape.columns),
        tensor.rows(), tensor.columns());

    Sweep swept;
    const std::size_t base = linear_algebra::span_of(field, tensor.slices).dimension();
    search_children::below(field, tensor.slices, pool, canon, tensor.slices, base, base + depth,
                           [&](const std::vector<bilinear_rank::Matrix>& child) {
                               compare_on(canon, bilinear_rank::pool_inside(field, pool, child),
                                          swept.visited);
                           });
    for (std::size_t which = 0; which < random_sets; ++which) {
        compare_on(canon, random_set(source, pool.size(), 1 + which % 8), swept.random);
    }
    return swept;
}

/// Both halves of one shape's sweep, named so a failure says which spoke.
void report_on(const std::string& shape, const Sweep& swept) {
    std::cout << "  " << shape << ": " << swept.visited.cells << " cells from the search's sets, "
              << swept.random.cells << " from random ones\n";
    check::equal(shape + ", enough cells to be a sample",
                 swept.visited.cells > 500 && swept.random.cells > 100 ? 1 : 0, 1);
    check::equal(shape + ", visited sets where the one-cell test disagrees with the loop",
                 swept.visited.disagreements, 0);
    check::equal(shape + ", random sets where the one-cell test disagrees with the loop",
                 swept.random.disagreements, 0);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";
    std::mt19937 source(20260820);

    // `⟨2,2,2⟩` to the target the module is pinned at, so the sets compared are
    // the sets the enumeration that publishes 36-in-1-orbit actually met. Three
    // levels: the base spans four dimensions and the target is seven.
    report_on("<2,2,2>", sweep(directory + "/matmul_2x2x2.tensor", {2, 2, 2}, 3, 200, source));

    // `⟨2,2,3⟩` two levels down rather than to its target: a second group acting
    // on a second pool is the point here, not a second full enumeration.
    report_on("<2,2,3>", sweep(directory + "/matmul_2x2x3.tensor", {2, 2, 3}, 2, 200, source));

    return check::report("the distinguished cell, one call against the loop");
}
