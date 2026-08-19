// Does naming an orbit from generators agree with naming it by walking the group?
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

#include "candidate_pool.h"
#include "check.h"
#include "group_construction.h"
#include "pool_set_canon.h"
#include "subspace_canon.h"
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
    std::vector<bilinear_rank::SubspaceCode> by_walking;
    std::vector<std::vector<std::size_t>> by_generators;
    for (std::size_t index = 0; index < pool.size(); ++index) {
        std::vector<bilinear_rank::Matrix> child = tensor.slices;
        child.push_back(pool[index]);

        by_walking.push_back(bilinear_rank::canonical_subspace(field, whole, child).code);
        by_generators.push_back(canon.canonical(bilinear_rank::pool_inside(field, pool, child)));
    }

    const std::vector<std::size_t> walked = partition_of(by_walking);
    const std::vector<std::size_t> generated = partition_of(by_generators);

    check::equal("the two agree on every one of the 225 children", generated == walked ? 1 : 0, 1);

    std::size_t orbits = 0;
    for (const std::size_t label : walked) orbits = label + 1 > orbits ? label + 1 : orbits;
    std::printf("depth-one orbits: %zu, by both routes\n", orbits);

    return check::report("pool set canon against the group walk");
}
