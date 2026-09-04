#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "dimacs_file.h"
#include "smtlib_file.h"

/// Running a solver, which is a program on the machine and not a library here.
///
/// Solvers are run as programs found on `PATH` rather than linked as libraries.
/// The build depends on Givaro and nothing else, and a machine without a solver
/// builds and tests fine. It just cannot answer this particular question, and
/// says so.
///
/// **That is the current arrangement, not a rule.** It has a price, and the
/// price is incremental solving. Every question re-encodes and re-solves from
/// scratch, so nothing learned at `k` is reused at `k+1`, and a scan over
/// candidate first terms cannot hand one solver the same base formula under a
/// new assumption. Buying that back means linking a solver, through IPASIR or a
/// process kept alive, and `cadical` is in the Ubuntu archive. What the
/// arrangement buys in exchange is that each question is a separate
/// deterministic process, so its cost does not depend on the order it was
/// reached in, and that is what makes the schedule pricing in
/// [`bracket/`](bracket/) exact rather than indicative. Whether the trade is
/// still the right one is open and remains unsettled here.
///
/// Three are looked for, each for a reason:
///
/// | | Why it is here |
/// |---|---|
/// | `cryptominisat` | native XOR clauses, which is exactly the shape of a GF(2) tensor equation |
/// | `kissat` | the strongest solver available on unsatisfiable instances, which is where proving a lower bound lives |
/// | `cvc5` | the theory of finite fields, so GF(p) needs no encoding at all |
///
/// **Kissat is tried first, and that is a measurement rather than a guess.** A
/// GF(2) tensor equation is a parity constraint, so CryptoMiniSat taking it as
/// one line instead of four clauses ought to win; on these instances it is
/// worth nothing at all, 1.559 s against 1.563 s on the same question. Kissat's
/// raw strength on unsatisfiable instances is worth five times, and the
/// expensive questions here are exactly the ones that answer no. Numbers in
/// [`method/`](method/).
namespace satisfiability {

/// A solver found on `PATH`, and whether it understands XOR clauses directly.
struct SatSolver {
    bool found = false;
    std::string name;
    std::string path;
    bool native_xor = false;
    /// Whether this solver takes a proof file as its second argument, which is
    /// kissat's interface and not a universal one. False means a proof cannot be
    /// had from it, and `solve` refuses the request rather than dropping it: a
    /// refusal reported with `--proof` given and no proof written is the false
    /// confidence the flag exists to remove.
    bool writes_proofs = false;
};

/// The order the solvers are looked for in, when a caller names none.
///
/// A function and not a constant because it is also the compiled default of the
/// argument below, and one of the two would otherwise be a copy of the other.
const std::vector<std::string>& default_solver_order();

/// Look for a SAT solver. `prefer_xor` asks for one that takes parity
/// constraints natively, which is only worth it on GF(2); `named` pins a
/// choice instead of taking the preference; `order` is which to try first, of
/// those on `PATH`.
///
/// `order` is an argument rather than read from `tunables.conf` here, because a
/// library that opens a config file binds every caller to a working directory.
/// The command reads the file and fills `SolveOptions::solver_order`; an empty
/// order means `default_solver_order`.
SatSolver find_sat_solver(bool prefer_xor, const std::string& named = "",
                          const std::vector<std::string>& order = default_solver_order());

/// Where `drat-trim` is, or empty. Without it a proof can be written but not
/// checked, and a refusal stays a matter of trusting the solver.
std::string find_proof_checker();

/// Where `cvc5` is, or empty. Its finite-field solver needs a CoCoALib build,
/// which distribution packages have been known to omit, so a solver that is
/// present may still refuse the query. That refusal comes back as no verdict.
std::string find_smt_solver();

/// What became of a refutation.
///
/// `Refuted` is the one that matters: the solver said unsatisfiable and its own
/// proof does not check out. That is a bug in the encoding or the solver, never
/// a lower bound, and it must stop the program rather than colour a line of
/// output.
enum class Proof { NotAsked, Written, Verified, Refuted };

struct SolverRun {
    bool solver_found = false;
    std::string solver_name;
    /// Bytes of the DRAT proof written (an unsatisfiability proof checkable by
    /// a program that shares no code with the solver), when a proof was asked
    /// for and the verdict was unsatisfiable. Zero otherwise.
    std::size_t proof_bytes = 0;
    Proof proof = Proof::NotAsked;
    /// False when the solver was found but gave no verdict: killed by the
    /// timeout or the memory cap, or refusing the theory. **Never read this as
    /// unsatisfiable**, because that would turn giving up into a proof of a
    /// lower bound.
    bool answered = false;
    bool satisfiable = false;
    formats::Model model;
    formats::SmtModel field_model;
    double seconds = 0;
};

/// Write the formula out and solve it, under a memory cap and a wall clock.
///
/// Both limits are arguments rather than constants because the only machine
/// this has run on shares its memory with other long searches, and a solver
/// that takes the box down has answered nothing.
/// `proof_path`, when given, receives a DRAT refutation of the formula, and it
/// is an error to give it to a solver that writes none.
///
/// This is the only thing here that makes a "no" checkable. A "yes" carries its
/// own certificate, since the decomposition can be multiplied out; a "no" is a
/// claim about everything the solver did not visit, and without a proof it rests
/// entirely on the solver being correct. A DRAT file can be checked by a program
/// that shares no code with the solver that produced it.
/// Which way to bias a solver that can be biased.
///
/// kissat ships two configurations, and its own help gives what they expand to:
/// `--sat` is `--target=2 --restartint=50`, `--unsat` is `--stable=0`. They fit
/// this problem exactly, because a sweep knows in advance which answer it
/// expects: every question below the rank is a refusal and only the last is a
/// find. Off by default until the table says otherwise, on the rule that
/// nothing here becomes a default without a measurement behind it.
enum class Tuning { None, Satisfiable, Unsatisfiable };

SolverRun run_solver(const formats::Cnf& formula, const SatSolver& solver,
                std::size_t memory_megabytes = 2048, std::size_t timeout_seconds = 300,
                const std::string& proof_path = "", Tuning tuning = Tuning::None);

/// The same for an SMT problem in the theory of finite fields.
SolverRun run_smt_solver(const formats::SmtProblem& problem,
                         std::size_t memory_megabytes = 2048,
                         std::size_t timeout_seconds = 300);

}  // namespace satisfiability
