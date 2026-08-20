#include "isomorph_rejection.h"

#include <algorithm>

namespace bilinear_rank {

namespace {

OrbitTest chosen_test = OrbitTest::Full;

}  // namespace

void set_orbit_test(OrbitTest test) { chosen_test = test; }
OrbitTest orbit_test() { return chosen_test; }

bool least_in_orbit(const PoolAction& action, const std::vector<std::uint32_t>& residual,
                    std::uint32_t point, std::uint32_t from) {
    // `seen` is a list rather than a set because an orbit inside a live suffix is
    // small and a linear scan of it beats hashing. This replaced a `struck` array
    // and a `position` table, both one word or one byte per pool element per
    // depth; at `⟨4,4,4⟩` each was 17.2 GB against 16 GB of memory, so the
    // quotient could not run there at all.
    std::vector<std::uint32_t> seen{point};
    std::vector<std::uint32_t> frontier{point};
    while (!frontier.empty()) {
        const std::uint32_t reached = frontier.back();
        frontier.pop_back();
        for (const std::uint32_t element : residual) {
            const std::uint32_t image = action.image(element, reached);
            if (image >= from && image < point) return false;
            if (std::find(seen.begin(), seen.end(), image) != seen.end()) continue;
            seen.push_back(image);
            frontier.push_back(image);
        }
    }
    return true;
}

bool least_under_generators(const PoolAction& action, const std::vector<std::uint32_t>& residual,
                            std::uint32_t point, std::uint32_t from) {
    // The loop above with the closure taken out: images of `point` itself and of
    // nothing else. The guard is the same one, and it has to be, or the two rules
    // would be asking about different suffixes and the containment argument in
    // the header would have nothing to stand on.
    for (const std::uint32_t element : residual) {
        const std::uint32_t image = action.image(element, point);
        if (image >= from && image < point) return false;
    }
    return true;
}

bool opens_a_branch(const PoolAction& action, const std::vector<std::uint32_t>& residual,
                    std::uint32_t point, std::uint32_t from) {
    return chosen_test == OrbitTest::Generators
               ? least_under_generators(action, residual, point, from)
               : least_in_orbit(action, residual, point, from);
}

}  // namespace bilinear_rank
