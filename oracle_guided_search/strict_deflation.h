#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "fixed_rank_finder.h"

/// The same commitment as [the finder](fixed_rank_finder.h), with the acceptance
/// test strengthened until it is minimality-preserving, and priced.
///
/// The finder passes a candidate over when its budget runs out. This one insists
/// on **proving** that a candidate cannot belong to any `k`-term decomposition
/// before discarding it, so an exhausted sweep here is a refutation of
/// `rank <= k` rather than a shrug. That is the whole difference, and it is
/// entirely a difference in what the rejections cost.
///
/// **It was expected to lose and it does not.** The objection on record is that
/// each refusal costs 28 to 44 s, more than the 25.8 s lower-bound proof it sits
/// inside. Neither number survives matched flags: the whole-instance refutation of
/// `⟨2,2,2⟩` at `k = 6` costs **1.32 s** and the five pinned refutations cost
/// **1.65 s** together. Full table in
/// [`refutation-prices.md`](refutation-prices.md).
///
/// **The real result is which machine buys the refutation.** Two of them, asked
/// about the same candidates in the same order:
///
/// - **The solver.** `decide_rank` on the cube, waiting for unsatisfiable. At
///   `⟨2,2,2⟩` and `k = 6`, 0.29 to 0.35 s per candidate with `break_symmetry`, and
///   1.63 to 2.67 s without, so the ordering alone is worth about 7x here.
/// - **The quotiented tree.** `expand_subspace_up_to_symmetry` (`[covanov2019]` Algorithm 3)
///   on `span(T) + t`, with the stabiliser recomputed for that enlarged span.
///   **0.0085 to 0.0099 s per candidate, 45 to 72 nodes**, which is 34x the whole
///   pinned solver sweep and 27x the whole-instance refutation.
///
/// The reason is structural rather than lucky, and it says where this stops working.
/// Pinning raises the dimension of the space the tree starts from, so it removes a
/// *level* of the tree: `span(T) + t` has dimension 5 and the target is 6, so one
/// level remains and 45 orbits is the entire search. Pinning does nothing
/// comparable to a CNF instance's difficulty. So the tree wins exactly when
/// `k - dim(span(T) + t)` is small, and at `⟨3,3,3⟩` with `k = 23` that depth is 13
/// and the advantage is gone.
///
/// The tree route refutes something slightly stronger than the cube does: the cube
/// pins `t` as a term, the tree only requires `t` to lie in the span. Refuting the
/// weaker statement refutes the stronger one, so the rejection stays sound. It is
/// the sound direction of the two and worth saying which way it points.
namespace bilinear_rank {

/// How a candidate is to be refuted.
enum class Refuter {
    /// `decide_rank` on the cube, waiting for unsatisfiable.
    Solver,
    /// `expand_subspace_up_to_symmetry` on the span enlarged by the candidate.
    QuotientedTree,
};

/// How the strict step should ask.
struct StrictSettings {
    /// Seconds per candidate. A refutation needs the large budget, unlike a find.
    std::size_t candidate_seconds = 300;
    Refuter refuter = Refuter::Solver;
    /// Nodes the tree route may visit before giving up. Exhausting it is an
    /// `Unknown`, never a refutation, exactly as `SearchBudget::exhausted` says.
    std::size_t node_limit = 5'000'000;
    /// Ask the candidates on several cores. They share nothing, so this changes
    /// wall time and no verdict. Bounded: five candidates at 30 s is 44 s wall
    /// rather than 103 s, and at depth two the longest single cube measured 143 s,
    /// more than the whole question, so a bad split of the work cannot be rescued
    /// this way.
    bool parallel_candidates = false;
    std::vector<std::size_t> matmul_shape;
    satisfiability::Approach approach;
};

/// One candidate's verdict, with what it cost to reach.
struct CandidateVerdict {
    std::size_t candidate = 0;
    /// `Yes` accepts, `No` is a proved rejection, `Unknown` is neither and leaves
    /// the step unable to conclude.
    satisfiability::Verdict verdict = satisfiability::Verdict::Unknown;
    double seconds = 0;
    /// Tree route only, and the number that says whether a `No` is a refutation or
    /// a budget: `exhausted` false means the limit was hit and the verdict is
    /// `Unknown`.
    std::size_t nodes = 0;
};

/// What one strict step established.
struct StrictStep {
    std::size_t products = 0;
    /// True when some candidate was accepted, which by the collapse also means the
    /// whole decomposition is in hand.
    bool accepted = false;
    /// True when every candidate was **refuted**, with none left undecided: this
    /// is a lower bound, `rank > products`, up to the group.
    bool refuted = false;
    std::vector<CandidateVerdict> verdicts;
    /// Which solver answered, empty on the tree route. Named for the same reason
    /// `FoundAtRank` names it: the price of a refutation is a property of the
    /// solver as much as of the question.
    std::string solver_name;
    double seconds = 0;
    std::vector<Matrix> decomposition;
    Algorithm algorithm;
};

/// Ask every orbit representative in turn, accepting the first that answers and
/// proving the rest impossible.
///
/// Stops at the first acceptance. Throws `cli::CheckFailed` if an accepted
/// decomposition does not rebuild the map.
StrictStep strict_step(const linear_algebra::Tensor& tensor, std::size_t products,
                       const StrictSettings& settings);

}  // namespace bilinear_rank
