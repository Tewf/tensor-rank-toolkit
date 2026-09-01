/// The projection bound: sound, cheap, and beaten by what this repository
/// already has.
///
/// Asserted here is the only property that must hold, soundness: the bound never
/// exceeds a rank that is known independently. A lower bound that is too large is
/// a false refutation, which is the one error nothing downstream could catch.
///
/// That it is *weaker* than `rank_lower_bound` is measured rather than asserted,
/// and recorded in [`../projection-bound.md`](../projection-bound.md). A test
/// pinning it as weaker would fail the day somebody strengthens it, which is the
/// opposite of what a test is for.
#include <string>
#include <vector>

#include "check.h"
#include "projection_lower_bound.h"
#include "rank_lower_bound.h"
#include "tensor_file.h"

namespace {

struct Fixture {
    const char* name;
    long long rank;  ///< known independently, by exhaustion or from the literature
};

constexpr Fixture kFixtures[] = {
    {"f2_2x2", 3}, {"f2_2x3", 5}, {"gf8_multiplication", 6}, {"matmul_2x2x2", 7},
};

/// The least `R` the projection argument does not refute.
std::size_t projection_floor(const linear_algebra::ModularField& field,
                             const std::vector<linear_algebra::ModularMatrix>& slices) {
    for (std::size_t products = 1; products <= 40; ++products) {
        if (!pencil_rank::projections_refute(field, slices, products)) return products;
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    for (const Fixture& fixture : kFixtures) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + fixture.name + ".tensor");
        const linear_algebra::ModularField field(tensor.characteristic);
        const std::string label = fixture.name;
        const long long floor = static_cast<long long>(projection_floor(field, tensor.slices));

        check::equal(label + ": the bound does not exceed the rank", floor <= fixture.rank ? 1 : 0,
                     1);

        // Every refutation it makes must be one the rank really does refute. This
        // is the same claim from the other side and catches an off-by-one in the
        // `products - axis + 2` target that the first check would not.
        for (long long products = 1; products < floor; ++products) {
            check::equal(label + ": refusing " + std::to_string(products) + " is right",
                         products < fixture.rank ? 1 : 0, 1);
        }
    }

    // Fewer than three slices leaves the projection nothing to remove, and the
    // answer must be "undecided" rather than a bound of 1.
    {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/w_state.tensor");
        const linear_algebra::ModularField field(tensor.characteristic);
        check::equal("two slices: refutes nothing",
                     pencil_rank::projections_refute(field, tensor.slices, 1) ? 1 : 0, 0);
    }

    return check::report("projection lower bound");
}
