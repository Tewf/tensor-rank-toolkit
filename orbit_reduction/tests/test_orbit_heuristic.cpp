/// That quotienting the heuristic's pool does not change where it stops.
///
/// `orbit_heuristic.h` argues it cannot: at the fixed point no representative
/// improves the map, improving is invariant under the map's own stabiliser, so no
/// member of the whole pool improves it either, and the quotiented walk therefore
/// has the same stopping condition as the plain one. That is the whole
/// justification for the quotient, and nothing ran it. `minimise_rank_up_to_symmetry` had
/// no test of any kind.
///
/// Only two fixtures can be asked. The quotient needs an ambient group, and
/// `all_automorphisms` builds one only where both general linear groups are
/// small, which is the 2x2 and 2x3 maps and nothing else here. The matrix
/// multiplication shapes go through the closed form instead and are covered by
/// their own commands.
#include <iostream>
#include <string>
#include <vector>

#include "candidate_pool.h"
#include "check.h"
#include "group_construction.h"
#include "measures.h"
#include "minimise_rank.h"
#include "orbit_heuristic.h"
#include "span_queries.h"
#include "tensor_file.h"

namespace {

using bilinear_rank::Field;
using bilinear_rank::Matrix;

/// Steps 2 and 3 as `minimise-rank` runs them, against the whole pool.
std::vector<Matrix> plain(const Field& field, const std::vector<Matrix>& slices,
                          const std::vector<Matrix>& pool) {
    std::vector<Matrix> current = bilinear_rank::descend_from_own_basis(field, slices);
    const std::vector<Matrix> shortlist =
        bilinear_rank::improving_candidates(field, current, pool);
    return bilinear_rank::minimise_rank(field, current, shortlist);
}

/// The same, with step 3 taking one candidate per orbit.
std::vector<Matrix> quotiented(const Field& field, const std::vector<Matrix>& slices,
                               const std::vector<Matrix>& pool,
                               bilinear_rank::OrbitReport* report) {
    const std::vector<Matrix> start = bilinear_rank::descend_from_own_basis(field, slices);
    const std::vector<bilinear_rank::Automorphism> ambient = bilinear_rank::all_automorphisms(
        field, slices.front().rows(), slices.front().columns());
    return bilinear_rank::minimise_rank_up_to_symmetry(field, start, pool, ambient, report);
}

struct Fixture {
    const char* name;
    long long rank;  // what both walks should reach on this map
};

constexpr Fixture kFixtures[] = {{"f2_2x2", 3}, {"f2_2x3", 5}};

}  // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cout << "usage: test_orbit_heuristic <fixtures>\n";
        return 1;
    }
    const std::string directory = argv[1];

    for (const Fixture& fixture : kFixtures) {
        const formats::Tensor tensor =
            formats::read_tensor_file(directory + "/" + fixture.name + ".tensor");
        const Field field(tensor.characteristic);
        const std::vector<Matrix> pool =
            bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

        const std::vector<Matrix> without = plain(field, tensor.slices, pool);
        bilinear_rank::OrbitReport report;
        const std::vector<Matrix> with = quotiented(field, tensor.slices, pool, &report);

        const std::string label = fixture.name;
        const long long plain_cost =
            static_cast<long long>(linear_algebra::multiplication_count(field, without));
        const long long orbit_cost =
            static_cast<long long>(linear_algebra::multiplication_count(field, with));

        check::equal(label + ": the plain walk reaches the rank", plain_cost, fixture.rank);
        check::equal(label + ": the quotiented walk stops in the same place", orbit_cost,
                     plain_cost);

        // A quotient that lost the map would be a far worse failure than one that
        // lost products, and neither walk is allowed to.
        check::equal(label + ": and still generates the map",
                     linear_algebra::spans_all(field, with, tensor.slices) ? 1 : 0, 1);

        // The quotient has to be doing something, or the agreement above is
        // agreement with itself.
        check::equal(label + ": the pool is the one the report saw",
                     static_cast<long long>(report.pool), static_cast<long long>(pool.size()));
        const bool reduced = !report.orbits.empty() && report.orbits.front() < pool.size();
        check::equal(label + ": and it really did collapse the pool", reduced ? 1 : 0, 1);
        std::cout << "        " << label << ": " << pool.size() << " candidates to "
                  << report.orbits.front() << " orbits under a stabiliser of "
                  << report.stabiliser_size.front() << "\n";
    }

    return check::report("orbit heuristic");
}
