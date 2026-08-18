#pragma once

#include <vector>

#include "bilinear_rank_aliases.h"

/// Step 1, and the only step of the heuristic that guarantees anything.
///
/// Choosing a basis of `span(T)` whose ranks sum to the least is a **matroid**
/// problem: linear independence forms a matroid, and greedy by ascending weight
/// yields a minimum-weight basis (Rado-Edmonds). So this does not approximate,
/// and its answer does not depend on how ties are broken. What is heuristic is
/// the constraint that the answer be a basis of `span(T)` at all, which is
/// exactly what [steps 2 and 3](minimise_rank.h) relax.
namespace bilinear_rank {

/// Walk the span of the map from lowest rank upwards, greedily keeping anything
/// not already spanned, until a full basis is assembled.
///
/// The result spans exactly what it was given, so it computes the same bilinear
/// map, and its ranks sum to no more than the original's.
///
/// `ranks_without_last` is an optimisation, and optional: the ranks of the span
/// of every slice but the last, in enumeration order. Steps 2 and 3 call this
/// once per candidate with the map fixed and the candidate appended, so the
/// `p^(k)` elements whose coefficient on the candidate is zero are the same
/// elements every single time. Handing them over skips both the combination and
/// the rank for that half of the enumeration.
std::vector<Matrix> minimum_weight_basis(const Field& field, const std::vector<Matrix>& slices,
                                   const std::vector<std::size_t>& ranks_without_last = {});

/// The basis of `slices` with one more map thrown in: the answer to "what would
/// adopting this candidate cost?".
///
/// Both walks over a candidate pool ask exactly this, so it lives here with the
/// step it calls rather than twice in the two callers.
std::vector<Matrix> minimum_weight_basis_with(const Field& field, const std::vector<Matrix>& slices,
                               const Matrix& candidate,
                               const std::vector<std::size_t>& known = {});

/// The rank of every element of the span, indexed the way `coefficient_vector`
/// indexes it, for feeding back in above.
std::vector<std::size_t> span_element_ranks(const Field& field,
                                            const std::vector<Matrix>& slices);

}  // namespace bilinear_rank
