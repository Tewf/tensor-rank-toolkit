#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// The heuristic run against a pool quotiented by the map's own symmetries.
///
/// [Steps 2 and 3](../descent_search/minimise_rank.h) walk a pool of rank-one maps and adopt the
/// first candidate that lowers the count. If `σ` maps `span(T)` to itself then
/// adding `φ` and adding `φ∘σ` cost exactly the same, so one of each orbit is
/// enough and the rest are the same experiment repeated.
///
/// **The catch, and why this is not a one-line change.** The heuristic has
/// state: after an adoption its map is no longer `T` but something larger, and
/// the group that made two candidates interchangeable was the group of `T`. A
/// quotient taken once goes stale the moment the map moves, and a stale quotient
/// can throw away the candidate that would have paid. So the group is recomputed
/// from the map every time the map moves, which is the same intersection
/// Covanov's Algorithm 3 carries down its recursion, and cheap here because
/// adoptions are rare.
///
/// At the fixed point no representative improves the map, and since improving is
/// invariant under the map's own stabiliser, no member of the whole pool
/// improves it either. That is the same stopping condition the unquotiented walk
/// has, which is what "without making the answer worse" means.
namespace bilinear_rank {

/// What the quotient did, for the write-up: it is worth knowing whether a pool
/// of 225 collapsed to 6 orbits or stayed at 225.
struct OrbitReport {
    std::size_t pool = 0;
    std::vector<std::size_t> stabiliser_size;  // one entry per round
    std::vector<std::size_t> orbits;           // one entry per round
};

/// Steps 2 and 3, quotiented. `ambient` is the group to take the map's
/// stabiliser from, and `pool` must be closed under it.
std::vector<Matrix> minimise_rank_up_to(const Field& field, std::vector<Matrix> slices,
                                        const std::vector<Matrix>& pool,
                                        const std::vector<Automorphism>& ambient,
                                        OrbitReport* report);

}  // namespace bilinear_rank
