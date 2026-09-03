#pragma once

#include <vector>

#include "bilinear_rank_aliases.h"
#include "candidate_pool.h"

/// Steps 2 and 3: recombining a map with rank-one maps from outside its own
/// basis, which is where the heuristic stops guaranteeing anything.
///
/// Adopt a candidate whenever the basis of the enlarged span costs fewer
/// multiplications than the map costs now. Enlarging is allowed: the result only
/// has to *generate* the original map, so a spanning set of more slices but
/// lower total rank is a better answer, which is exactly what Karatsuba's five
/// products for a four-coefficient product is.
///
/// The two steps differ only in the pool they are handed: step 2 uses the map's
/// own rank-one products, step 3 every rank-one map of the shape.
namespace bilinear_rank {

/// Keep only the candidates that, taken one at a time, would improve the map.
///
/// Named for what it does: keep only the candidates that would improve the map
/// when taken one at a time.
std::vector<Matrix> improving_candidates(const Field& field, const std::vector<Matrix>& slices,
                                         const std::vector<Matrix>& candidates);

/// The same, over a pool addressed rather than built.
///
/// Same candidates, same order, same answer: `RankOnePool::at(i)` is element `i`
/// of what `all_rank_one_maps` returns, which
/// [`candidate_pool.h`](candidate_pool.h) states and
/// [its test](tests/test_candidate_pool.cpp) asserts. What changes is the room.
/// This walk keeps no index once it is past, so the only thing that has to exist
/// at once is the shortlist it hands back, which is the few candidates that pay
/// rather than the `(p^rows - 1)(p^columns - 1)/(p-1)^2` that were offered.
///
/// A caller holding a pool for some other reason should pass the vector: the two
/// overloads differ in where the maps come from and in nothing else.
std::vector<Matrix> improving_candidates(const Field& field, const std::vector<Matrix>& slices,
                                         const RankOnePool& pool);

/// Adopt improvements until no candidate is left that pays.
std::vector<Matrix> minimise_rank(const Field& field, std::vector<Matrix> slices,
                                  std::vector<Matrix> candidates);

/// Steps 1 and 2 together: the minimum-rank basis, then descent over the
/// candidates that basis generates itself.
///
/// The cheap half of the heuristic, and it was written out three times. Both
/// `decide-rank --anchor heuristic` and `walk-scheme --from` need a decent
/// starting point rather than a report of how they got there, which is the one
/// thing `minimise-rank` cannot delegate: it prints each step as it goes.
std::vector<Matrix> descend_from_own_basis(const Field& field,
                                           const std::vector<Matrix>& slices);

}  // namespace bilinear_rank
