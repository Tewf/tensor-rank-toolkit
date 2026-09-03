#include "orbit_heuristic.h"

#include <numeric>

#include "measures.h"
#include "minimise_rank.h"

namespace bilinear_rank {

std::vector<Matrix> minimise_rank_up_to_symmetry(const Field& field, std::vector<Matrix> slices,
                                        const std::vector<Matrix>& pool,
                                        const std::vector<Automorphism>& ambient,
                                        OrbitReport* report) {
    if (report != nullptr) report->pool = pool.size();

    std::vector<std::uint32_t> everything(pool.size());
    std::iota(everything.begin(), everything.end(), std::uint32_t(0));

    for (;;) {
        // The group of the map as it stands now, not of the map we started
        // with. This is the line that keeps the quotient honest.
        const std::vector<Automorphism> stabiliser = stabiliser_of(field, slices, ambient);
        const std::vector<std::uint32_t> representatives =
            orbit_representatives(permutation_action_on(field, stabiliser, pool), everything);

        if (report != nullptr) {
            report->stabiliser_size.push_back(stabiliser.size());
            report->orbits.push_back(representatives.size());
        }

        std::vector<Matrix> quotiented;
        quotiented.reserve(representatives.size());
        for (const std::uint32_t index : representatives) quotiented.push_back(pool[index]);

        const std::size_t before = linear_algebra::multiplication_count(field, slices);
        std::vector<Matrix> improved = minimise_rank(
            field, slices, improving_candidates(field, slices, quotiented));

        // No representative pays, so nothing in the pool does: whatever the walk
        // would have tried is equivalent to something it just tried.
        if (linear_algebra::multiplication_count(field, improved) >= before) return slices;
        slices = std::move(improved);
    }
}

}  // namespace bilinear_rank
