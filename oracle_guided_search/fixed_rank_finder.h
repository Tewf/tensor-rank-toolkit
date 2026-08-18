#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "bilinear_rank_aliases.h"
#include "rank_question.h"
#include "tensor_file.h"

/// Finding a `k`-term decomposition without ever waiting for a refutation.
///
/// [`decide_rank`](../satisfiability/rank_question.h) answers "is there one of
/// size `k`". This asks only the questions it expects to be satisfiable, gives each
/// a small budget, and moves on when the budget runs out.
///
/// **The reason it was built does not survive being measured, and that is recorded
/// here rather than in a footnote.** The design assumes a large accept/reject
/// asymmetry: a yes at 0.15 to 0.23 s against a no at 28 to 44 s, quoted as 190x to
/// 210x. Re-measured with **matched flags** on this machine, a pinned refutation at
/// `⟨2,2,2⟩` and `k = 6` costs **0.29 to 0.35 s** and a pinned acceptance at `k = 7`
/// costs **0.44 s**, so the asymmetry is about **one**. The earlier figures came
/// from cube literals appended by hand to an instance encoded without the symmetry
/// breaking the whole-instance run had. Everything measured is in
/// [`measurements.md`](measurements.md) and
/// [`refutation-prices.md`](refutation-prices.md).
///
/// So this module is kept for what it does provide, not for the premise it was
/// built on: a descending schedule, a polynomial pre-test, and honest verdicts. It
/// is **dominated by `plateau_search` and `minimise_rank` on every fixture
/// measured**, and nothing here should be reached for before those.
///
/// **The trade, stated once, because it is the whole design.** A candidate whose
/// question exhausts its budget is *passed over*. That loses completeness and
/// never soundness: what comes back is a decomposition, multiplied out against
/// the map by `recovers_map` before it is handed over, so a yes is a yes. An
/// exhausted sweep reports `Outcome::NotFound`, which is **not** "no
/// decomposition of this size exists", and nothing may read it as one. This is
/// the same discipline `Verdict::Unknown` keeps in the oracle, carried one layer
/// up.
///
/// The single place a real no can come from is the floor, and it comes from
/// outside this file: a lower bound proved in polynomial time refuses every `k`
/// below it without asking anything.
///
/// **The collapse, noted rather than papered over.** A satisfying model is the
/// *whole* decomposition, not one term of it, so the first accepting candidate
/// ends the search outright. There is no term-by-term deflation loop here and
/// there is nothing for one to do: committing to a candidate is a restriction on
/// the search, not a step of a construction. Anything that recomputed the orbits
/// after an acceptance would be recomputing them for an iteration that never
/// comes.
///
/// Related and deliberately not this: `[covanov2019]` Algorithm 3, implemented as
/// `expand_subspace_up_to_symmetry` in [the quotiented search](../orbit_reduction/orbit_search.h),
/// which is complete and exponential. Keys are [references.md](../references.md).
namespace bilinear_rank {

/// How the finder should ask.
struct FinderSettings {
    /// Seconds allowed per candidate. Exhausting it passes the candidate over.
    ///
    /// The sweep's worst case is `candidates x this`, and only the accepting call
    /// has to finish, so a small budget is what makes the sweep affordable rather
    /// than a way of hurrying the solver.
    std::size_t candidate_seconds = 30;

    /// A lower bound on the rank, proved elsewhere, below which `k` is refused
    /// without a solver.
    ///
    /// A number and not a computation, so the bound can be improved without
    /// touching this file. `linear_algebra::flattening_lower_bound` is the free
    /// one and what the command passes. Yang's `ranksum` (`[yang2025]`) is
    /// stronger and is being written elsewhere; when it lands it goes in here and
    /// nothing else changes. Zero means no floor.
    std::size_t floor = 0;

    /// The matrix multiplication shape, when the tensor is one.
    ///
    /// The closed-form orbits of `⟨n, m, k⟩` are what supply the candidates.
    /// Named and never inferred from the slice dimensions, because `orbit_cubes`
    /// checks the map really is that product and that check is the only guard
    /// against pinning the first term to a map this tensor lacks. Empty means no
    /// commitment: one unrestricted question per `k`.
    std::vector<std::size_t> matmul_shape;

    /// Skip the polynomial pre-test when the full rank-one pool would exceed this
    /// many maps.
    ///
    /// The test needs the whole pool, not one map per orbit, since a
    /// representative of a class need not be the member lying inside the span. At
    /// `⟨3,3,3⟩` that pool is 261 121 matrices of 81 entries, 184 MB, for a test
    /// that cannot fire on a matrix multiplication tensor at all.
    std::size_t pool_limit = 100'000;

    /// Passed to the oracle. `timeout_seconds` is overwritten with
    /// `candidate_seconds`, so setting it here has no effect.
    satisfiability::Approach approach;
};

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

/// Ask for a `products`-term decomposition of `tensor`, committing to one orbit
/// representative at a time.
///
/// Throws what the oracle throws, and `cli::CheckFailed` if a decomposition the
/// solver certified fails to rebuild the map through `recovers_map`. That second
/// one is a bug in this repository, never a result.
FoundAtRank find_at_rank(const linear_algebra::Tensor& tensor, std::size_t products,
                         const FinderSettings& settings);

}  // namespace bilinear_rank
