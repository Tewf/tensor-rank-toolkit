#pragma once

#include <cstddef>
#include <vector>

#include "integer_programme.h"
#include "interpolation_programme.h"

/// The same minimisation, handed to a solver instead of enumerated.
///
/// `integer_programme.h` says this in its own opening paragraph: *"the
/// interpolation bound of the curve strand is an integer programme written out by
/// hand. This is the model they can be handed to instead."* So this is that
/// module's documented purpose being taken up, not a new idea.
///
/// **Why a suboptimal solve is safe here, and would not be in a rank
/// refutation.** `[rambaud2014, Thm. 2]` reads
/// `µ_sym_q(m) ≤ Σᵢ µ_sym_q(deg Pᵢ, uᵢ)`, so *any*
/// feasible selection is already a valid bound and minimising only makes it the
/// tightest one the table allows. A solver that stops early yields a weaker
/// envelope, never a wrong one. Contrast a rank refutation, where an early stop
/// would be a false lower bound. The caveat is therefore claim strength, not
/// correctness, and it is discharged by recording provenance: see
/// `BoundResult::solved_by` and `BoundResult::optimum_proved`.
///
/// **This is a separate library target from `curve_bounds`.** A static library
/// propagates even a private dependency as a link requirement, so folding the
/// solver into `curve_bounds` would put `integer_programme/` on the include path
/// of everything that ever links the table. Commit `169a152` established that,
/// and the same manoeuvre is used here: one translation unit, one target.
namespace curve_bounds {

/// Which solvers may answer.
enum class SolverChoice {
    /// `solve()`: the first installed backend whose point the model accepts,
    /// falling through to the built-in. Fastest, and an outside solver's
    /// `Optimal` is not a proof of optimality.
    Chain,
    /// `branch_and_bound` directly, which is what `solve_with(BuiltIn, ...)` is.
    /// Slower, exact, and the only verdict here that is a proof. Also the only
    /// route that reaches no shell: `solve()` probes `PATH` for each backend.
    BuiltInOnly,
};

/// Theorem 2's right-hand side as a model: one integer variable per priced
/// `(degree, multiplicity)` pair the supply can reach.
///
/// Four things the model has to reproduce or it is not the same question:
///
/// - the degree row is an **equality**. A `≤` would always answer "take one
///   rational point, cost 1", which is a bound on nothing;
/// - a multiplicity the table does not price is **omitted as a column**, never
///   costed zero: `symmetric_upper_bound` returns 0 for *no published bound*, and
///   zero is the absence of a bound rather than a cheap one;
/// - `available` caps **distinct points** of a degree, so it is one `≤` row per
///   supply entry over that entry's own columns; multiplicities are not pooled;
/// - `chosen` is a multiset collapsed by `(degree, multiplicity)` with a count,
///   which is exactly what an integer variable per pair holds.
///
/// The whole decision space is at most 25 variables and at most `supply.size() + 1`
/// rows, because the table caps at degree 10 and multiplicity 10 and prices only
/// 25 of those hundred cells.
integer_programme::IntegerProgramme interpolation_programme_of(const std::vector<PointSupply>& supply,
                                                          std::size_t divisor_degree);

/// Minimise through a solver, falling back to the dynamic programme.
///
/// `Optimal` is used. `Infeasible` is believed, and matches the dynamic
/// programme's `solved == false`: only the built-in ever claims it, because
/// `solve()` treats an outside solver's infeasibility as one more decline.
/// `Exhausted` and `Unbounded` fall back, which is where the dynamic programme
/// earns its keep as more than a cross-check.
///
/// "No solver available" is not among the outcomes and needs no branch:
/// `Backend::BuiltIn` is unconditionally available and `solve()` ends there, so
/// the chain cannot be empty. What can happen is a budget running out, which is
/// `Exhausted`, and that is a different thing from infeasible.
BoundResult minimise_interpolation_bound_by_solver(const std::vector<PointSupply>& supply,
                                                 std::size_t divisor_degree,
                                                 SolverChoice choice = SolverChoice::Chain);

/// Nodes the built-in may open. Config, not code.
///
/// It is the only budget that reaches the built-in: `set_solver_time_limit` is a
/// `timeout` prefix for outside solvers and `branch_and_bound` has no wall clock
/// anywhere. Reaching this returns `Exhausted`, which falls back rather than
/// bounding anything.
std::size_t solver_node_limit();
void set_solver_node_limit(std::size_t nodes);

}  // namespace curve_bounds
