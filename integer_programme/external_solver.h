#pragma once

#include "integer_programme.h"
#include "solver_chain.h"

/// Running an installed solver on a written-out model and reading its answer
/// back.
///
/// What comes back is a point in decimal, so it is treated as a candidate rather
/// than as a result: it is read onto exact rationals, whole variables are put
/// back on the integers their decimals were a rendering of, and the whole thing
/// is put through `satisfies` before anyone sees it. A backend that returns
/// nothing usable is indistinguishable here from one that is not installed, and
/// that is the intended shape: both mean "ask the next".
///
/// The solver is started with `fork` and `execvp` in a process group of its own,
/// never through a shell, so `solver_time_limit()` can be enforced by killing that
/// group rather than hoped for. Nothing of ours is running when this returns, which
/// is the property the previous `std::system` under `timeout` did not have.
///
/// Verified against the installed binaries on this machine: lp_solve 5.5, CBC
/// 2.10.11, GLPK 5.0. The Gurobi recipe follows its documented `ResultFile`
/// output and is unverified, there being no licence here to test it with.
namespace optimisation {

Solution run_backend(Backend backend, const IntegerProgramme& programme);

}  // namespace optimisation
