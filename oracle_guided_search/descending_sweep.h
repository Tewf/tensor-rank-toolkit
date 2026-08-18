#pragma once

#include <cstddef>
#include <vector>

#include "fixed_rank_finder.h"

/// Walking `k` down from the naive ceiling, so the upper bound half of the
/// pipeline never asks for a refutation.
///
/// [`find_rank`](../satisfiability/rank_question.h) walks *up* from the floor, so
/// every question but the last is unsatisfiable and pays refusal prices the whole
/// way. That is the right schedule when the floor is tight, which on these
/// fixtures it usually is. It is the wrong one when it is not: on GF(16) the
/// flattening bound is five short and the walk has to buy every refusal in
/// between.
///
/// Descending pays the opposite prices. Every question but the last is
/// satisfiable, and the last one is not waited for. What comes out is an upper
/// bound `U` and a floor from outside, and handing `[floor, U]` back to `find_rank`
/// leaves it exactly one refutation to buy, at `U - 1`, instead of one per rank
/// from the floor upward.
///
/// **What this is not.** The sweep stops at the first `k` it cannot reach, and that
/// `k` is *not* thereby impossible: see the trade in
/// [`fixed_rank_finder.h`](fixed_rank_finder.h). `U` is an upper bound and the
/// stopping point is a budget, not a bound. Nothing here may be read as a lower
/// bound, and the sweep returns no lower bound of its own for that reason.
namespace bilinear_rank {

/// Computing every output coordinate separately: always achievable, so always an
/// upper bound, and where a descending sweep has to start.
std::size_t naive_ceiling(const linear_algebra::Tensor& tensor);

/// The result of walking down.
struct SweepResult {
    /// The floor as it was handed in, repeated so `[floor, upper]` can be read off
    /// one object.
    std::size_t floor = 0;
    /// The least `k` a verified decomposition was found at. Zero when none was,
    /// which happens only if even the naive ceiling could not be reached.
    std::size_t upper = 0;
    /// One entry per `k` attempted, in descending order, so the cost of the sweep
    /// is itemised rather than summed away.
    std::vector<FoundAtRank> steps;
    double seconds = 0;

    /// The decomposition at `upper`, multiplied out against the map already.
    std::vector<Matrix> decomposition;
    Algorithm algorithm;
};

/// Walk `k` down from `ceiling` while decompositions keep being found.
///
/// Stops on the first `k` that is not found, or on reaching `settings.floor`,
/// below which no `k` can be found and asking would waste the budget.
SweepResult descend_from_ceiling(const linear_algebra::Tensor& tensor, std::size_t ceiling,
                                 const FinderSettings& settings);

}  // namespace bilinear_rank
