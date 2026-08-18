#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "integer_programme.h"

/// Which solver answers, given what this machine actually has.
///
/// The ranking is fixed and the availability is not, so a machine with nothing
/// installed still solves its programmes, more slowly, and a machine that
/// acquires Gurobi tomorrow uses it without a line changing. A solver that is
/// absent is a state reported here, never an error discovered downstream.
///
/// Backends are found on `PATH` at run time and never linked, which is how the
/// satisfiability strand already treats `kissat` and `cvc5`. It means the tree
/// builds identically everywhere and that `--solver` is a run-time choice rather
/// than a build-time one.
///
/// **A `no` is only ever believed from the built-in.** An outside solver's point
/// can be checked against the model and is; its claim that no point exists
/// cannot be, so `solve` treats that claim as one more backend declining to
/// answer and carries on down the chain. That makes proving infeasibility as
/// slow as the exact solver, on purpose: a false `Infeasible` is the one answer
/// here that nothing downstream would catch.
namespace optimisation {

/// Best first. Gurobi leads where a licence exists, and the built-in trails
/// because it is the slowest and the only one that never needs checking.
enum class Backend { Gurobi, Cbc, Glpk, LpSolve, BuiltIn };

const std::vector<Backend>& ranked_backends();

std::string name_of(Backend backend);
Backend backend_named(const std::string& name, bool& recognised);

/// How long an outside solver may run before it is stopped, in seconds.
///
/// Config rather than code, and not optional. Five leaked solvers once sat at
/// full tilt for half an hour and quietly spoiled another session's measurements.
///
/// This used to be a `timeout N` prefix on a shell command, which bounded an
/// orphan's *duration* and not its effect: for those seconds it still held a core
/// and every later measurement read slow. `external_solver.cpp` now forks, puts
/// the child in a process group of its own, keeps the pid and kills the **group**
/// at the deadline, so nothing of ours outlives the call. The limit bounds the
/// wait; the group kill is what ends the run.
///
/// **It does not reach the built-in.** `branch_and_bound` has no wall clock
/// anywhere, so a caller forcing that backend is bounded by `node_limit` alone.
unsigned solver_time_limit();
void set_solver_time_limit(unsigned seconds);

/// Is this backend's binary on `PATH`? The built-in always is.
bool is_available(Backend backend);

/// Those of `ranked_backends` that are, best first.
std::vector<Backend> available_backends();

/// The first installed backend whose answer the model accepts, falling through
/// to the built-in. `node_limit` bounds the built-in only.
Solution solve(const IntegerProgramme& programme, std::size_t node_limit);

/// One named backend, installed or not. `Exhausted` when it is not.
Solution solve_with(Backend backend, const IntegerProgramme& programme, std::size_t node_limit);

}  // namespace optimisation
