#pragma once

#include <cstddef>
#include <string>
#include <vector>

/// A solver that can only find, and the line it is run with.
///
/// A stochastic local search solver flips variables until every constraint
/// holds or it is stopped, so it has two answers and not three: a model, or
/// nothing. It never refutes. That makes it a Las Vegas algorithm, right
/// whenever it answers and random in how long it takes, and a cost from it is
/// a distribution over seeds rather than a number.
///
/// `[heule2019]` ran one, yalsat, on the `<3,3,3>` instances and found that
/// "local search SAT solvers outperform CDCL solvers consistently in this
/// application". The record on those instances is `[nawrocki2021]`'s xnfSAT,
/// which is yalsat with the parities kept as parities inside the flip loop:
/// a GF(2) tensor equation is a parity, expanded parities are where
/// WalkSAT-type solvers fail (`[jia2004]`, `[haanpaa2006]`,
/// `[riccitersenghi2010]`), and the linear 3-cut expansion `--plain-cnf`
/// writes is the worst of the eight CNF encodings they measured. Which of
/// these this repository's encoding is closer to is what
/// [`las-vegas/`](las-vegas/README.md) measures.
///
/// Four are known here, by the name of their binary, and each has its own
/// command line. None takes a proof file, so `--proof` is refused upstream.
/// Only xnfsat reads an `x` line; the other three are always handed the
/// parities expanded, whatever the formula preferred.
///
/// | | Where from | Reads `x` lines | Seed | Budget of its own |
/// |---|---|---|---|---|
/// | `yalsat` | `[biere2018]`, the solver `[heule2019]` used | no | second positional argument | none: the wall clock kills it |
/// | `xnfsat` | `[nawrocki2021]`, yalsat with native XOR; starts from the all-false assignment by default, which that paper found better than random; an `x` line is odd parity, the same convention as cryptominisat and this repository's writer; run with `--witness=1`, since it prints no model otherwise | **yes** | second positional argument | none: the wall clock kills it |
/// | `probSAT` | `[balint2012]` | no | second positional argument | none: the wall clock kills it |
/// | `multilinear-sat` | a continuous relaxation with restarts, local to this machine; no XOR support yet | no | `--seed` | `--time-limit`, so it says `s UNKNOWN` itself; run under `OMP_NUM_THREADS=1`, since its CPU backend otherwise takes every core |
///
/// Their restart policies are left at each solver's default and not made a
/// tunable: yalsat's is an inner interval under reluctant doubling, probSAT's a
/// count of flips per try, multilinear-sat's a Luby unit in gradient iterations,
/// so one number would mean three things, and the baselines `[heule2019]` and
/// `[nawrocki2021]` set are yalsat and xnfsat at their defaults.
namespace satisfiability {

/// Whether a solver of this name answers yes or nothing.
bool finds_only_by_name(const std::string& name);

/// Whether a solver of this name takes a parity as one `x` line. The formula
/// keeps its parities apart from its clauses until it is written, so this is
/// the whole decision between the XNF and the 3-cut expansion.
bool reads_xor_lines_by_name(const std::string& name);

/// The argument vector for one of them: the binary, the formula, the seed, and
/// the clock where the solver has a flag for it.
///
/// `multilinear-sat` is pinned to its CPU backend and to one thread. The timing
/// protocol in `MEASURING.md` is one core, and a figure taken on the card is a
/// different measurement with rules of its own.
std::vector<std::string> local_search_command(const std::string& name, const std::string& binary,
                                              const std::string& file, std::size_t seed,
                                              std::size_t timeout_seconds);

}  // namespace satisfiability
