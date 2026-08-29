#pragma once

#include <cstddef>
#include <string>
#include <vector>

/// A solver that can only find, and the line it is run with.
///
/// A stochastic local search solver flips variables until every clause holds or
/// it is stopped, so it has two answers and not three: a model, or nothing. It
/// never refutes. That makes it a Las Vegas algorithm, right whenever it
/// answers and random in how long it takes, and a cost from it is a
/// distribution over seeds rather than a number.
///
/// `[heule2019]` ran one, yalsat, on the `<3,3,3>` instances and found that
/// "local search SAT solvers outperform CDCL solvers consistently in this
/// application". The other side of the field's record is that WalkSAT-type
/// solvers fail on parity constraints (`[jia2004]`, `[haanpaa2006]`,
/// `[riccitersenghi2010]`), and a GF(2) tensor equation is a parity. Which of
/// the two this encoding is closer to is what [`las-vegas/`](las-vegas/README.md)
/// measures.
///
/// Three are known here, by the name of their binary, and each has its own
/// command line. None reads an `x` line, so the parities are always expanded;
/// none takes a proof file, so `--proof` is refused upstream.
///
/// | | Where from | Seed | Budget of its own |
/// |---|---|---|---|
/// | `yalsat` | `[biere2018]`, the solver `[heule2019]` used | second positional argument | none: the wall clock kills it |
/// | `probSAT` | `[balint2012]` | second positional argument | none: the wall clock kills it |
/// | `multilinear-sat` | a continuous relaxation with restarts, local to this machine | `--seed` | `--time-limit`, so it says `s UNKNOWN` itself |
///
/// Their restart policies are left at each solver's default and not made a
/// tunable: yalsat's is an inner interval under reluctant doubling, probSAT's a
/// count of flips per try, multilinear-sat's a Luby unit in gradient iterations,
/// so one number would mean three things, and the baseline `[heule2019]` sets
/// is yalsat at its defaults.
namespace satisfiability {

/// Whether a solver of this name answers yes or nothing.
bool finds_only_by_name(const std::string& name);

/// The argument vector for one of them: the binary, the formula, the seed, and
/// the clock where the solver has a flag for it.
///
/// `multilinear-sat` is pinned to its CPU backend. The timing protocol in
/// `MEASURING.md` is one core, and a figure taken on the card is a different
/// measurement with rules of its own.
std::vector<std::string> local_search_command(const std::string& name, const std::string& binary,
                                              const std::string& file, std::size_t seed,
                                              std::size_t timeout_seconds);

}  // namespace satisfiability
