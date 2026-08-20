// Is the subgroup that comes back the stabiliser, or something around it?
//
// `stabiliser_generators` is the other half of what the factored presentation has
// to answer, and it is the half where being wrong in one direction is not merely
// slow. Too small a stabiliser leaves the enumerator generating several children
// per orbit: more nodes, same answer. Too large a one merges augmentations that are
// not equivalent, so a subspace is never generated and a refutation is claimed on
// fewer subspaces than the count says — the failure nothing downstream catches, and
// the reason the cheap answer (stabilise the touched rows and the touched columns,
// one `setStabilizer` call away) is refused in `pool_set_canon.h`.
//
// So the check is set equality and not order equality. At `⟨2,2,2⟩` the group is
// 216 elements, so `Stab_G(S)` is available as a list by definition: filter the 216
// by whether they carry `S` to `S`. The subgroup the returned generators actually
// generate is closed out by multiplication. Two sets of permutations of the 225
// pool elements, compared element for element, which pins the order as a corollary
// and pins rather more than the order.
//
// Both kinds of set are asked about: the pool contents the search's own tree
// produces, which are spans and not arbitrary, and random subsets, which are the
// sets no span produces and which is where a set stabiliser has room to be wrong.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "group_construction.h"
#include "map_construction.h"
#include "pool_action_table.h"
#include "pool_set_canon.h"
#include "pool_sets.h"
#include "search_children.h"
#include "span_basis.h"

namespace {

using Permutation = std::vector<std::uint32_t>;

/// The image of `cells` under one permutation of the pool, sorted as `cells` is.
std::vector<std::size_t> moved(const Permutation& permutation,
                               const std::vector<std::size_t>& cells) {
    std::vector<std::size_t> image;
    image.reserve(cells.size());
    for (const std::size_t cell : cells) image.push_back(permutation[cell]);
    std::sort(image.begin(), image.end());
    return image;
}

/// `Stab_G(cells)` by definition: the elements of the group that carry the set to
/// itself, as distinct permutations of the pool.
///
/// Distinct permutations rather than group elements, because the group's map into
/// `Sym(pool)` need not be injective and the thing on the other side of the
/// comparison is a set of permutations.
std::set<Permutation> stabiliser_by_walking(const std::vector<Permutation>& whole,
                                            const std::vector<std::size_t>& cells) {
    std::set<Permutation> fixing;
    for (const Permutation& element : whole) {
        if (moved(element, cells) == cells) fixing.insert(element);
    }
    return fixing;
}

/// Everything the returned generators generate, by closing under multiplication.
///
/// Forward products only: the group is finite, so a set closed under the generators
/// is closed under their inverses too. `ceiling` stops a wrong generating set from
/// enumerating `Sym(225)` rather than reporting a failure.
std::set<Permutation> generated_by(const std::vector<Permutation>& generators,
                                   std::size_t points, std::size_t ceiling) {
    Permutation identity(points);
    for (std::size_t point = 0; point < points; ++point) {
        identity[point] = static_cast<std::uint32_t>(point);
    }

    std::set<Permutation> reached{identity};
    std::vector<Permutation> frontier{identity};
    while (!frontier.empty() && reached.size() <= ceiling) {
        const Permutation element = frontier.back();
        frontier.pop_back();
        for (const Permutation& generator : generators) {
            Permutation product(points);
            for (std::size_t point = 0; point < points; ++point) {
                product[point] = generator[element[point]];
            }
            if (reached.insert(product).second) frontier.push_back(product);
        }
    }
    return reached;
}

struct Tally {
    long long sets = 0;
    long long disagreements = 0;
    long long overflowed = 0;
    long long largest = 0;
};

void compare_on(const bilinear_rank::PoolSetCanon& canon, const std::vector<Permutation>& whole,
                const std::vector<std::size_t>& cells, Tally& tally) {
    const std::set<Permutation> truth = stabiliser_by_walking(whole, cells);
    const std::set<Permutation> generated =
        generated_by(canon.stabiliser_generators(cells), canon.size(), whole.size());

    ++tally.sets;
    if (generated.size() > whole.size()) ++tally.overflowed;
    if (generated != truth) ++tally.disagreements;
    tally.largest = std::max<long long>(tally.largest, static_cast<long long>(truth.size()));
}

}  // namespace

int main() {
    const pool_sets::MatmulShape shape{2, 2, 2};
    const bilinear_rank::Field field(2);
    const std::vector<bilinear_rank::Matrix> slices =
        bilinear_rank::matrix_multiplication_tensor(shape.rows, shape.inner, shape.columns);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, shape.slice_rows(), shape.slice_columns());
    const std::vector<bilinear_rank::Automorphism> generators =
        bilinear_rank::matrix_multiplication_symmetry_generators(field, shape.rows, shape.inner,
                                                                 shape.columns);
    const bilinear_rank::PoolSetCanon canon(field, generators, shape.slice_rows(),
                                            shape.slice_columns());
    const std::vector<std::vector<std::uint32_t>> whole = pool_action_table::of(
        field,
        bilinear_rank::matrix_multiplication_symmetries(field, shape.rows, shape.inner,
                                                        shape.columns),
        shape.slice_rows(), shape.slice_columns());

    check::equal("the group the oracle walks", static_cast<long long>(whole.size()), 216);

    Tally tally;
    // The empty set first, which is the enumerator's own root: nothing of the pool
    // lies inside the target subspace, so the stabiliser asked for is the whole
    // group and the answer has to be the whole group and not a piece of it.
    compare_on(canon, whole, {}, tally);

    const std::size_t base = linear_algebra::span_of(field, slices).dimension();
    search_children::below(field, slices, pool, canon, slices, base, base + 3,
                           [&](const std::vector<bilinear_rank::Matrix>& child) {
                               compare_on(canon, whole,
                                          bilinear_rank::pool_inside(field, pool, child), tally);
                           });

    std::mt19937 source(20260822);
    for (std::size_t which = 0; which < 400; ++which) {
        compare_on(canon, whole, pool_sets::random_set(source, pool.size(), 1 + which % 9),
                   tally);
    }

    std::cout << "  " << tally.sets << " sets, largest stabiliser " << tally.largest << "\n";
    check::equal("enough sets to be a sample", tally.sets > 400 ? 1 : 0, 1);
    check::equal("sets where the generated subgroup is not the stabiliser",
                 tally.disagreements, 0);
    check::equal("sets where the generators left the group entirely", tally.overflowed, 0);

    return check::report("the stabiliser against the walk of all 216 elements");
}
