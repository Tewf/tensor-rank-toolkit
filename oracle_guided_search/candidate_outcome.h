#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "bilinear_rank_aliases.h"
#include "rank_question.h"

/// What one committed question came back with.
///
/// These types were part of `fixed_rank_finder.h`, whose search was measured to
/// be dominated on every fixture and now lives on the `rejected-experiments`
/// branch. They stayed when it was removed, on the premise that
/// `strict_deflation`, which asks the same shape of question one candidate at a
/// time against a budget, would report through them. It does not: `StrictStep`
/// reports through `satisfiability::Verdict` and its own `accepted`/`refuted`
/// flags, and neither `Outcome` nor `FoundAtRank` is referenced anywhere else in
/// this repository. Left in place rather than removed, since that is a decision
/// past what a comment edit should make.
namespace bilinear_rank {

/// How one `k` ended. The last two are not the same claim and must not be merged.
enum class Outcome {
    /// A decomposition was found and multiplied out against the map.
    Found,
    /// Found by the polynomial pre-test, with no solver run at all.
    FoundWithoutSolver,
    /// `k` is below a bound proved elsewhere, so this is a genuine no.
    BelowFloor,
    /// Every candidate was passed over. **Not** a no.
    NotFound,
};

/// What one `k` cost and what it produced.
struct FoundAtRank {
    Outcome outcome = Outcome::NotFound;
    std::size_t products = 0;
    /// Candidates available, and how many were asked about before this ended.
    std::size_t candidates = 0;
    std::size_t candidates_asked = 0;
    /// The index of the candidate that answered, when one did.
    std::size_t accepted_candidate = 0;
    /// One entry per candidate asked, in orbit order, so the sweep can be priced
    /// rather than merely timed.
    satisfiability::CubeReport per_candidate;
    /// What the oracle said about the whole split, kept beside the outcome rather
    /// than collapsed into it.
    ///
    /// `No` means every candidate refused with none timing out, which is a real
    /// refutation because the cubes cover every first term up to the group.
    /// `Unknown` means at least one candidate was passed over, so nothing is
    /// proved either way. Both arrive as `NotFound`, and only this tells them
    /// apart. A finder that does not wait for refutations will almost always show
    /// `Unknown`, which is the design working and not a failure.
    satisfiability::Verdict oracle_verdict = satisfiability::Verdict::Unknown;
    /// Which solver answered. Reported because this repository prices questions in
    /// seconds and two solvers do not charge the same: the same pinned refutation
    /// costs kissat and cryptominisat different amounts, and a timing without a
    /// solver beside it has already been published here once and had to be
    /// corrected.
    std::string solver_name;
    double seconds = 0;
    std::vector<Matrix> decomposition;
    /// The three operators, recovered and checked. Set on either `Found`.
    Algorithm algorithm;
};

inline bool was_found(Outcome outcome) {
    return outcome == Outcome::Found || outcome == Outcome::FoundWithoutSolver;
}

}  // namespace bilinear_rank
