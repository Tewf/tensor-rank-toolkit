#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"
#include "pool_orbits.h"

/// How every element of a group moves every pool element, written out.
///
/// The table is exactly what the factored presentation exists to avoid (at
/// `⟨4,4,4⟩` one row of it is 17 GB), and having one here is the point: a check on
/// a canonical form wants an oracle that walks the group element by element, and
/// walking wants the images laid out rather than divided out. `⟨2,2,2⟩`'s is 216 by
/// 225.
///
/// The arithmetic is not written a second time. Each row comes from
/// [`pool_orbits.h`](../../orbit_reduction/pool_orbits.h)'s `PoolAction`, which is
/// the repository's one statement of where an automorphism sends a rank-one map,
/// and which
/// [`test_orbit_cubes.cpp`](../../orbit_reduction/tests/test_orbit_cubes.cpp)
/// already holds against
/// `permutation_action_on`'s independent table on all 48 600 images at `⟨2,2,2⟩`.
namespace pool_action_table {

inline std::vector<std::vector<std::uint32_t>> of(
    const bilinear_rank::Field& field, const std::vector<bilinear_rank::Automorphism>& elements,
    std::size_t rows, std::size_t columns) {
    const std::size_t points = bilinear_rank::normalised_vectors(field, rows).size() *
                               bilinear_rank::normalised_vectors(field, columns).size();
    const bilinear_rank::PoolAction action(field, elements, rows, columns);

    std::vector<std::vector<std::uint32_t>> table;
    table.reserve(action.size());
    for (std::size_t which = 0; which < action.size(); ++which) {
        std::vector<std::uint32_t> moves(points);
        for (std::size_t point = 0; point < points; ++point) {
            moves[point] = action.image(which, static_cast<std::uint32_t>(point));
        }
        table.push_back(std::move(moves));
    }
    return table;
}

}  // namespace pool_action_table
