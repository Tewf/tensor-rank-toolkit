/// The factorisation, checked against ranks established elsewhere and against
/// its own receipt.
///
/// Two things are asserted and they fail differently. That the component count
/// is the rank is a claim about the search, and it is checked against numbers
/// this repository settled by other routes. That `C A` is the tensor is a claim
/// about the answer, and it is checked by multiplying, which is why a wrong
/// answer here cannot look like a right one.
#include <string>
#include <vector>

#include "check.h"
#include "factorisation.h"
#include "measures.h"
#include "solver_process.h"
#include "tensor_file.h"

namespace {

struct Fixture {
    const char* name;
    long long rank;
    const char* why;
};

constexpr Fixture kFixtures[] = {
    {"f2_2x2", 3, "Karatsuba"},
    {"f2_2x3", 5, "the write-up's worked example"},
    {"gf4_multiplication", 3, "GF(4) over GF(2)"},
    {"w_state", 3, "rank 3, border rank 2"},
    {"pencil_split_f3_3", 3, "diagonalisable, so its size"},
    {"pencil_nilpotent_f2_3", 4, "a Jordan block costs its size plus one"},
};

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    for (const Fixture& fixture : kFixtures) {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/" + fixture.name + ".tensor");
        const linear_algebra::ModularField field(tensor.characteristic);
        const std::string label = fixture.name;

        canonical_factorisation::FactorisationSettings settings;
        const canonical_factorisation::Factorisation factorisation =
            canonical_factorisation::factor_over_canonical_basis(field, tensor.slices, settings);

        check::equal(label + ": components, " + fixture.why,
                     static_cast<long long>(factorisation.components), fixture.rank);
        check::equal(label + ": and the sweep below them was complete",
                     factorisation.minimal ? 1 : 0, 1);
        check::equal(label + ": C A gives the slices back",
                     canonical_factorisation::recovers_slices(field, tensor.slices, factorisation)
                         ? 1 : 0,
                     1);

        // A is r by nm and C is k by r, which is the only shape that lets the
        // product be S at all. Asserted separately so a shape bug does not
        // arrive disguised as an arithmetic one.
        check::equal(label + ": A is as wide as the canonical basis",
                     static_cast<long long>(factorisation.chosen.columns()),
                     static_cast<long long>(tensor.rows() * tensor.columns()));
        check::equal(label + ": C is as tall as there are slices",
                     static_cast<long long>(factorisation.recovery.rows()),
                     static_cast<long long>(tensor.slices.size()));

        // The floor has to be a floor. It is a proved bound, so a floor above
        // the answer would be a false lower bound rather than a slow search.
        check::equal(label + ": the floor does not exceed the answer",
                     factorisation.floor <= factorisation.components ? 1 : 0, 1);
    }

    // The two routes must agree, and they share almost nothing: one walks a tree
    // over a materialised pool, the other hands clauses over the operand vectors
    // to somebody else's solver and never enumerates a rank-one map. A shape
    // where they disagree is a defect in whichever is wrong, and the check costs
    // one extra solve on tensors this small.
    //
    // Skipped when no solver is on PATH, and skipped loudly: a route that was
    // never run reported as agreeing is the worst outcome available here.
    if (satisfiability::find_sat_solver(false).found) {
        for (const Fixture& fixture : kFixtures) {
            const linear_algebra::Tensor tensor =
                linear_algebra::read_tensor_file(directory + "/" + fixture.name + ".tensor");
            const linear_algebra::ModularField field(tensor.characteristic);

            canonical_factorisation::FactorisationSettings by_solver;
            by_solver.route = canonical_factorisation::Route::Satisfiability;
            const canonical_factorisation::Factorisation solved =
                canonical_factorisation::factor_over_canonical_basis(field, tensor.slices,
                                                                     by_solver);

            check::equal(std::string(fixture.name) + ": the SAT route finds the same count",
                         static_cast<long long>(solved.components), fixture.rank);
            check::equal(std::string(fixture.name) + ": and its C A is the tensor too",
                         canonical_factorisation::recovers_slices(field, tensor.slices, solved) ? 1
                                                                                                : 0,
                         1);
        }
    } else {
        std::cout << "  skip  no SAT solver on PATH, so the routes were not compared\n";
    }

    // A factorisation that is tampered with must be refused. Without this the
    // check above only proves the checker says yes to something.
    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/f2_2x2.tensor");
        const linear_algebra::ModularField field(tensor.characteristic);
        canonical_factorisation::Factorisation factorisation =
            canonical_factorisation::factor_over_canonical_basis(
                field, tensor.slices, canonical_factorisation::FactorisationSettings{});

        // Flipping one entry of A breaks the product, and usually the rank-one
        // constraint with it. Either refusal is the right one.
        factorisation.chosen(0, 0) = factorisation.chosen(0, 0) == 0 ? 1 : 0;
        check::equal("a tampered A is refused",
                     canonical_factorisation::recovers_slices(field, tensor.slices, factorisation)
                         ? 1 : 0,
                     0);
    }

    return check::report("canonical basis factorisation");
}
