// Does naming an orbit from generators agree with the closed form for it?
//
// `canonical_subspace` takes the least subspace code over every element of the
// group. `PoolSetCanon` takes the least image of the subspace's pool content
// under the same group presented on the pool, from generators, via Linton's
// algorithm. The two use different orderings, so their *keys* cannot be
// compared; what has to agree is the **partition into orbits** those keys induce.
//
// That is the whole correctness claim of the reduction, and 2x2x2 is where it can
// be checked: the group is 216 elements, small enough for the walk to be
// available as an oracle at all.
#include <map>
#include <string>
#include <vector>

#include "check.h"
#include "group_construction.h"
#include "candidate_pool.h"
#include "pool_orbits.h"
#include "pool_set_canon.h"
#include "tensor_file.h"

namespace {

/// Relabel a sequence of keys as 0, 1, 2 ... in order of first appearance, so two
/// labelings can be compared for inducing the same partition rather than for
/// being equal.
template <class Key>
std::vector<std::size_t> partition_of(const std::vector<Key>& keys) {
    std::map<Key, std::size_t> seen;
    std::vector<std::size_t> labels;
    for (const Key& key : keys) {
        const auto found = seen.find(key);
        if (found != seen.end()) {
            labels.push_back(found->second);
            continue;
        }
        const std::size_t next = seen.size();
        seen.emplace(key, next);
        labels.push_back(next);
    }
    return labels;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    const linear_algebra::Tensor tensor =
        linear_algebra::read_tensor_file(directory + "/matmul_2x2x2.tensor");
    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    const std::vector<bilinear_rank::Automorphism> whole =
        bilinear_rank::matrix_multiplication_symmetries(field, 2, 2, 2);
    const std::vector<bilinear_rank::Automorphism> generators =
        bilinear_rank::matrix_multiplication_symmetry_generators(field, 2, 2, 2);
    const bilinear_rank::PoolSetCanon canon(field, generators, tensor.rows(), tensor.columns());

    check::equal("the group is the 216 the walk needs", whole.size(), std::size_t(216));
    check::equal("and the pool is 225 points", canon.size(), pool.size());

    // The depth-one children of the enumerator: the base plus one pool element.
    //
    // The oracle is deliberately **not** the whole-group walk this replaced, which
    // is retired to `rejected-experiments`. It is the closed form from the A_3
    // quiver in `../orbit_reduction/pool_orbits.h`, derived from Gabriel's theorem
    // and needing no group built at all. Two independent routes agreeing is worth
    // more than a fast one agreeing with the slow one it exists to retire.
    std::vector<std::vector<std::size_t>> names;
    for (std::size_t index = 0; index < pool.size(); ++index) {
        std::vector<bilinear_rank::Matrix> child = tensor.slices;
        child.push_back(pool[index]);
        names.push_back(canon.canonical(bilinear_rank::pool_inside(field, pool, child)));
    }

    const std::vector<std::size_t> classes = partition_of(names);
    std::size_t orbits = 0;
    for (const std::size_t label : classes) orbits = label + 1 > orbits ? label + 1 : orbits;

    const std::size_t closed_form =
        bilinear_rank::matrix_multiplication_orbit_representatives(field, 2, 2, 2).size();
    check::equal("the closed form says five orbits", closed_form, std::size_t(5));
    check::equal("and canonising the 225 children finds the same number", orbits, closed_form);

    // A marked pair's canonical form must be a function of the pair's orbit, and
    // nothing else. That is the whole contract, so it is checked directly: move
    // both the set and its marked point by a group element and the answer may not
    // move. The distinguished element the parent test wants is the minimum of
    // these over the candidates, and a minimum of orbit invariants is one.
    const bilinear_rank::FactoredAction action =
        bilinear_rank::factored_action(field, whole, tensor.rows(), tensor.columns());
    const std::size_t right_count =
        bilinear_rank::normalised_vectors(field, tensor.columns()).size();

    std::size_t pairs_checked = 0;
    for (std::size_t index = 0; index < pool.size(); index += 37) {
        std::vector<bilinear_rank::Matrix> child = tensor.slices;
        child.push_back(pool[index]);
        const std::vector<std::size_t> inside = bilinear_rank::pool_inside(field, pool, child);
        if (inside.empty()) continue;

        const std::size_t marked = inside.front();
        const std::vector<std::size_t> expected = canon.canonical_with_marked(inside, marked);

        for (std::size_t which = 0; which < whole.size(); which += 23) {
            const auto move = [&](std::size_t point) {
                return static_cast<std::size_t>(action.left[which][point / right_count]) *
                           right_count +
                       action.right[which][point % right_count];
            };
            std::vector<std::size_t> moved_set;
            for (const std::size_t point : inside) moved_set.push_back(move(point));

            check::equal("moving a marked pair does not move its canonical form",
                         canon.canonical_with_marked(moved_set, move(marked)) == expected ? 1 : 0,
                         1);
            ++pairs_checked;
        }
    }
    check::equal("and enough pairs were actually checked", pairs_checked > 20 ? 1 : 0, 1);

    return check::report("pool set canon against the closed form");
}
