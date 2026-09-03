#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// A walk that is allowed to cross plateaus: a plateau-crossing search.
///
/// This method is distinct from the [descent heuristic steps 2 and 3]
/// (../descent_search/minimise_rank.h) and published baseline methods because it guarantees
/// something different and its results should never be confused with theirs.
///
/// The reason it exists is a measurement. Steps 2 and 3 adopt a candidate only
/// when it makes the map **strictly** cheaper, and on matrix multiplication no
/// single rank-one map ever does: 0 of 225 candidates improve `⟨2,2,2⟩`, 0 of
/// 945 improve `⟨2,2,3⟩`, 0 of 32 193 improve `⟨2,3,3⟩`. The walk cannot take a
/// first step, and quotienting the pool by symmetry does not help, because an
/// empty shortlist stays empty however it is grouped.
///
/// Yet `⟨2,2,2⟩` has a 7-product algorithm, and the exact search finds it in
/// 0.0126 s (famous_tensors/decided-exactly.md; a fifth of a second before the
/// GF(2) leaf). Strassen's seven are reachable from the naive eight only
/// by passing through maps that cost the same. A strictly descending walk cannot
/// enter such a state by construction: this is a structural limit of the
/// method, not a contingent failure.
///
/// So this one takes a strict improvement whenever one exists, and otherwise
/// steps sideways to an equal-cost map it has not seen before, and it
/// **backtracks**, which a walk cannot. One sideways step is not enough: from
/// the naive eight, `⟨2,2,2⟩` needs three additions before the count moves, and
/// a linear walk dead-ends after four moves with nowhere new to go. It is a
/// depth-limited search over the plateau, not a walk across it.
///
/// The cheapest map ever seen is what comes back, so it can only match or beat
/// the walk it extends.
///
/// It guarantees nothing about optimality. It is a heuristic that can move where
/// the other one cannot.
namespace bilinear_rank {

/// What the walk did, since a plateau crossing is worth seeing.
struct PlateauReport {
    std::size_t improvements = 0;
    std::size_t sideways = 0;
    std::size_t states = 0;   // distinct subspaces visited
    std::size_t best = 0;     // the cheapest count reached
};

/// Search from `slices`, allowing `depth` consecutive equal-cost steps on any
/// one path and visiting at most `state_budget` distinct subspaces.
///
/// `ambient` may be empty, which means no quotient. When it is not, the pool is
/// quotiented by the current map's own stabiliser and re-quotiented whenever the
/// map moves, exactly as [the quotiented steps](../orbit_reduction/orbit_heuristic.h) do.
std::vector<Matrix> cross_plateaus(const Field& field, const std::vector<Matrix>& slices,
                                   const std::vector<Matrix>& pool,
                                   const std::vector<Automorphism>& ambient, std::size_t depth,
                                   std::size_t state_budget, PlateauReport* report);

}  // namespace bilinear_rank
