#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "solver_process.h"
#include "tensor_file.h"
#include "types.h"

/// Asking one question about a tensor's rank, and answering it with a solver.
///
/// The question is always the same shape: is there a decomposition of this
/// tensor into `products` rank-one terms? What changes is the field it is asked
/// over and therefore which of the three encodings states it, and that choice
/// is made here rather than at the command line, so that anything wanting an
/// answer gets the same dispatch.
///
/// Kept apart from [the command](commands/decide_rank_by_sat_main.cpp), which
/// parses arguments and sweeps `k` over a range. This decides one `k`.
namespace satisfiability {

/// How the question should be put.
struct SolveOptions {
    /// Hand `GF(p)` to cvc5's theory of finite fields instead of encoding it.
    bool use_field_theory = false;
    /// Write parities as clauses rather than `x` lines, for a solver without
    /// native XOR. Most solvers here have none, so this is usually what happens
    /// whether it is asked for or not.
    bool plain_cnf = false;
    /// Quotient by the symmetries a decomposition has: the order of its terms
    /// over any field, and the scaling of its operand vectors over GF(p).
    /// Sound, and worth a great deal on a question expected to answer no.
    bool break_symmetry = false;
    /// Pin a solver instead of taking the best fit.
    std::string solver;
    /// Bias the solver toward the answer this question is expected to have.
    /// See `Tuning` in [`solver_process.h`](solver_process.h): kissat ships the
    /// two configurations and a sweep knows which way each question leans.
    Tuning tuning = Tuning::None;

    /// One assignment of the first term's operand variables per orbit, as
    /// literals. Solving once per cube and taking the union is equivalent to
    /// solving the whole formula, provided the cubes really do cover every
    /// first term up to the group; a cube set that misses one turns a yes into
    /// a no. Empty means one undivided instance.
    ///
    /// **GF(2) only, and refused rather than merely documented** over any larger
    /// prime: the literals are numbered for the Boolean encoding, and the prime
    /// encoding constrains the very term a cube pins. `decide_rank` throws.
    std::vector<std::vector<int>> cubes;

    /// Set internally while solving one cube. Callers set `cubes`.
    std::vector<int> cube_literals;

    /// Where to write a DRAT refutation, when the answer is no. Empty means
    /// none, and a no is then believed on the solver's word alone.
    ///
    /// Asking for one from a solver that cannot write it, or from the field
    /// theory backend, is **refused rather than ignored**: it used to be dropped
    /// in silence, which left a refusal looking checked when nothing had checked
    /// it. Only kissat writes one here.
    std::string proof_path;

    std::size_t memory_megabytes = 2048;
    std::size_t timeout_seconds = 300;

    /// A smaller budget for the questions a search asks while navigating, as
    /// opposed to the one it cannot avoid. Zero uses `timeout_seconds` for
    /// everything.
    ///
    /// The point is not to answer faster but to spend the big budget once. Cost
    /// is concentrated just below the rank, so a probe that exhausts a small
    /// budget is itself evidence of being there, without that evidence being
    /// treated as an answer: an unknown stays unknown and moves no bound.
    std::size_t probe_seconds = 0;
};

/// What came back.
///
/// `Verdict::Unknown` is a third answer and never folded into `No`: a solver
/// killed by its timeout or its memory cap has not proved a lower bound, and
/// treating silence as refusal is how a search that gave up becomes a claim.
enum class Verdict { Yes, No, Unknown };

struct Answer {
    Verdict verdict = Verdict::Unknown;
    std::string solver_name;
    double seconds = 0;
    /// Size of the refutation written, when one was asked for and produced.
    std::size_t proof_bytes = 0;
    Proof proof = Proof::NotAsked;
    /// The decomposition, when there is one, checked against the tensor before
    /// it is handed back.
    std::vector<Matrix> decomposition;
};

/// How each cube of a split ended, for a caller that prices its candidates.
///
/// `decide_rank` returns one verdict for the whole split, which answers the
/// question asked and is the wrong thing to publish a table from: a sweep over
/// candidates needs to know *which* one paid and what the rest cost. Nullable and
/// filled in cube order, following `OrbitReport` in
/// [`orbit_heuristic.h`](../orbit_reduction/orbit_heuristic.h), which exists for
/// the same reason.
struct CubeReport {
    std::vector<Verdict> verdict;
    std::vector<double> seconds;
};

/// Decide whether `tensor` has a decomposition into `products` rank-one terms.
///
/// Throws when no solver is on `PATH`, when the approach and the field do not
/// go together, or when a solver answers yes with a model that does not rebuild
/// the tensor, which is a bug in the encoding rather than a result.
///
/// A cube split builds the base formula **once** and appends each cube's unit
/// clauses to a copy of it, rather than re-deriving it per cube. At `⟨3,3,3⟩` and
/// `r = 23` that formula is tens of thousands of clauses and there are thirteen
/// cubes, so the saving is real. It is encoding time only and buys nothing in the
/// solver, which is where the cost actually is.
Answer decide_rank(const linear_algebra::Tensor& tensor, std::size_t products,
                   const SolveOptions& approach, CubeReport* report = nullptr);

/// What a search established, and whether it is a determination.
struct RankBounds {
    /// Every `k` strictly below this was refused, so the rank is at least here.
    std::size_t lower = 0;
    /// A decomposition into this many was found, so the rank is at most here.
    std::size_t upper = 0;
    /// `lower == upper`, reached without any question going unanswered.
    bool exact = false;
    std::size_t questions_asked = 0;
    /// Refusals whose DRAT proof was checked and held. The proof file itself
    /// holds only the last one written, since each refusal overwrites it; the
    /// count is what says the rest were checked as they went.
    std::size_t refutations_verified = 0;
    double seconds = 0;
    std::vector<Matrix> decomposition;
};

/// Find the rank by walking up from the floor.
///
/// This is the MaxSAT literature's **linear search UNSAT-SAT**
/// (`[morgado2013]`), where every question but the last returns unsatisfiable.
/// That description makes it sound like the worst choice available, since
/// refusals are the dear ones, and elsewhere it often is. Here it wins, and the
/// reason is the free lower bound: the flattening rank is within three of the
/// answer on every fixture, so there are only a handful of refusals to pay and
/// they are the cheap ones far from the rank.
///
/// **Measured against descending, bisection and the two gallops**, on seven
/// fixtures at one ceiling each, in [`search.md`](search.md). It wins on the
/// cheap ones and **lost on the only expensive one, coming fourth of five on
/// GF(16)**, where the floor was five short so it walked through a 3.7 s question
/// no other schedule asks. That handicap is gone: the floor there is now 8 rather
/// than 4, since `[yang2025]`'s rank sums joined the flattenings in
/// [`rank_lower_bound.h`](../linear_algebra/rank_lower_bound.h), so the schedule
/// comparison predates the bound it depended on and wants re-measuring. It ties
/// the two mandatory questions on the four fixtures where the bound already
/// equals the rank. What it keeps
/// throughout is that its cost does not move when the ceiling is loosened,
/// because it never reads the ceiling, and the whole choice of schedule is worth
/// about three percent.
///
/// The upper bound is still wanted, as somewhere to stop.
RankBounds find_rank(const linear_algebra::Tensor& tensor, const SolveOptions& approach,
                     std::size_t floor, std::size_t ceiling);

/// Write the question to a file and stop, for a solver of your own.
///
/// Returns what was written, as "N variables, M clauses" or the SMT-LIB
/// equivalent, because the sizes are the interesting part of not solving it.
std::string write_question(const linear_algebra::Tensor& tensor, std::size_t products,
                           const SolveOptions& approach, const std::string& path);

}  // namespace satisfiability
