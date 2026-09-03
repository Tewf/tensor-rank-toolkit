/// The Kronecker structure and the closure bound, against ranks known without
/// them.
///
/// Every `rank` column below was settled by the exhaustive search, which shares
/// no code with this module. Two of the fixtures predate it; the other four were
/// written for it, and their ranks were measured rather than predicted, which is
/// how the gap this test now asserts was found in the first place.
#include <string>
#include <vector>

#include "check.h"
#include "kronecker_structure.h"
#include "tensor_file.h"

namespace {

struct Fixture {
    const char* name;
    long long rank;         ///< over GF(p), settled by exhaustion
    long long closure;      ///< what Ja'Ja's formula gives over the closure
    long long proved;       ///< the best proved bound, closure or `[sumi2009, Thm. 3.5]`
    bool settled;           ///< and whether it is proved to be the rank
};

/// Every two-slice tensor in `evidence/fixtures/`. Where `rank` exceeds `closure` the
/// field is too small for the classical construction, and the module says so
/// rather than reporting the smaller number as an answer.
constexpr Fixture kFixtures[] = {
    {"gf4_multiplication", 3, 2, 3, true},
    {"w_state", 3, 3, 3, true},
    {"pencil_nilpotent_f2_3", 4, 4, 4, false},
    {"pencil_split_f3_3", 3, 3, 3, true},
    {"pencil_singular_f2_2x3", 3, 3, 3, false},
    {"pencil_irreducible_f2_4", 6, 4, 5, false},
};

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    for (const Fixture& fixture : kFixtures) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + fixture.name + ".tensor");
        const pencil_rank::ModularField field(tensor.characteristic);
        const std::string label = fixture.name;

        const pencil_rank::PencilRank reported = pencil_rank::pencil_rank_of(field, tensor.slices);
        check::equal(label + ": the closure bound",
                     static_cast<long long>(reported.over_closure), fixture.closure);
        check::equal(label + ": the best proved bound",
                     static_cast<long long>(reported.proved), fixture.proved);
        check::equal(label + ": whether it is proved to be the rank",
                     reported.exact ? 1 : 0, fixture.settled ? 1 : 0);

        // Soundness is the property that must never break: a proved bound above
        // the rank is a false refutation, and nothing downstream would catch it.
        check::equal(label + ": and the proved bound does not exceed the rank",
                     static_cast<long long>(reported.proved) <= fixture.rank ? 1 : 0, 1);

        // The claim that makes the bound worth reporting. A bound above the rank
        // would be wrong outright, not merely a bound that happens to be loose.
        check::equal(label + ": and it does not exceed the rank",
                     static_cast<long long>(reported.over_closure) <= fixture.rank ? 1 : 0, 1);
        if (reported.exact) {
            check::equal(label + ": when settled, it is the rank",
                         static_cast<long long>(reported.proved), fixture.rank);
        }

        // The structure has to add up as well as produce a number. Both
        // dimension counts and the divisor degrees are checked inside
        // `kronecker_structure`, so reaching here is three identities holding;
        // what is asserted here is that the blocks fill the shape.
        const pencil_rank::KroneckerStructure structure =
            pencil_rank::kronecker_structure(field, tensor.slices[0], tensor.slices[1]);

        std::size_t width = structure.regular_size;
        for (const std::size_t index : structure.column_indices) width += index + 1;
        for (const std::size_t index : structure.row_indices) width += index;
        check::equal(label + ": the blocks are as wide as the tensor",
                     static_cast<long long>(width), static_cast<long long>(tensor.columns()));

        std::size_t height = structure.regular_size;
        for (const std::size_t index : structure.column_indices) height += index;
        for (const std::size_t index : structure.row_indices) height += index + 1;
        check::equal(label + ": and as tall",
                     static_cast<long long>(height), static_cast<long long>(tensor.rows()));
    }

    // A tensor with one slice is a matrix, and the pencil whose second slice is
    // zero has to agree with that rather than need a case of its own. The
    // identity is diagonalisable, so this one is exact.
    {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/pencil_nilpotent_f2_3.tensor");
        const pencil_rank::ModularField field(tensor.characteristic);
        const std::vector<linear_algebra::ModularMatrix> identity_only{tensor.slices[0]};
        const pencil_rank::PencilRank one_slice = pencil_rank::pencil_rank_of(field, identity_only);
        check::equal("one slice: the rank of the matrix",
                     static_cast<long long>(one_slice.over_closure), 3);
        check::equal("one slice: exactly", one_slice.exact ? 1 : 0, 1);
        check::equal("no slices: rank 0",
                     static_cast<long long>(pencil_rank::pencil_rank_of(field, {}).over_closure), 0);
    }

    return check::report("pencil rank");
}
