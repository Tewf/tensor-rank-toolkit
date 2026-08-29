#include "solver_process.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <unistd.h>

#include "child_process.h"
#include "interrupt_cleanup.h"
#include "local_search_solver.h"

namespace satisfiability {

namespace {

/// The first entry of `PATH` holding an executable called `name`.
std::string on_path(const std::string& name) {
    const char* path = std::getenv("PATH");
    if (path == nullptr) return {};

    std::istringstream entries(path);
    std::string entry;
    while (std::getline(entries, entry, ':')) {
        if (entry.empty()) continue;
        const std::filesystem::path candidate = std::filesystem::path(entry) / name;
        std::error_code ignored;
        if (std::filesystem::is_regular_file(candidate, ignored)) return candidate.string();
    }
    return {};
}

/// A scratch path nothing else in this process is using.
///
/// The process id alone was enough while one solver ran at a time. It is not
/// enough for a cube split with workers: every worker is this same process, so
/// every one of them would name the same file and they would overwrite each
/// other's formula. The counter is what makes the name unique per call.
std::filesystem::path scratch_file(const std::string& extension) {
    static std::atomic<unsigned long> issued{0};
    std::error_code ignored;
    return std::filesystem::temp_directory_path(ignored) /
           ("tensor-rank-" + std::to_string(::getpid()) + "-" +
            std::to_string(issued.fetch_add(1)) + extension);
}

/// A scratch path that removes itself however its scope is left.
///
/// `std::filesystem::remove` at the end of a function covers a return and not a
/// throw, and this module throws: `check_applicable` refuses a cube split it
/// cannot honour, and the formula is already on disk by then. The destructor
/// covers both, and `cli::remove_when_interrupted` covers the one route a
/// destructor cannot, a signal, which returns to nowhere.
class ScratchFile {
   public:
    explicit ScratchFile(std::filesystem::path path) : path_(std::move(path)) {
        cli::remove_when_interrupted(path_.string());
    }
    ~ScratchFile() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
        // Freed here and not merely removed: the interrupt table holds eight, and
        // every question registers a formula and a log, so a run that never gave
        // its slots back would refuse its fourth question.
        cli::forget_when_interrupted(path_.string());
    }
    ScratchFile(const ScratchFile&) = delete;
    ScratchFile& operator=(const ScratchFile&) = delete;

    const std::filesystem::path& path() const { return path_; }
    std::string string() const { return path_.string(); }

   private:
    std::filesystem::path path_;
};

/// Run an outside solver under a cap and hand back what it printed.
///
/// This used to be `popen("sh -c 'ulimit -v N; exec timeout T ...'")`, which is
/// the pattern `integer_programme` abandoned after measuring what it costs: a
/// `timeout` prefix bounds an orphan's duration and not its effect, and the
/// parent holds no handle on the process group, so a solver that forks leaves the
/// fork behind. One leak per worker is what a parallel cube split multiplies, so
/// this is a prerequisite for one rather than a tidy-up.
///
/// `run_limits/child_process.h` is now the one launcher in this repository. The
/// shell goes with it: the command is a vector, so nothing here has to quote a
/// path, `ulimit -v` becomes `setrlimit`, and `2>/dev/null` becomes
/// `merge_stderr` false, because this route parses the log and a solver's
/// progress chatter is not an answer.
std::string run_capped(const std::vector<std::string>& command, std::size_t megabytes,
                       std::size_t seconds) {
    const ScratchFile log(scratch_file(".log"));
    const bool started = bilinear_rank::run_to_completion(
        command, log.path(),
        bilinear_rank::ChildLimits{static_cast<double>(seconds), megabytes, false});

    std::string captured;
    if (started) {
        std::ifstream reading(log.path());
        std::ostringstream all;
        all << reading.rdbuf();
        captured = all.str();
    }
    return captured;
}

/// The argument vector for a solver, where `capped` used to build a shell line.
///
/// kissat takes the proof file as a second positional argument. Nothing else here
/// does, so the caller is told rather than the flag being guessed at.
std::vector<std::string> solver_command(const std::string& binary, const std::string& file,
                                        const std::string& proof = "",
                                        const std::string& configuration = "") {
    std::vector<std::string> command{binary};
    if (!configuration.empty()) command.push_back(configuration);
    command.push_back(file);
    if (!proof.empty()) command.push_back(proof);
    return command;
}

}  // namespace

const std::vector<std::string>& default_solver_order() {
    // Kissat first, whatever the parities look like. The reasoning that put
    // CryptoMiniSat ahead was that native XOR must be worth something on a
    // formula that is mostly parities; the measurement says it is worth
    // nothing here, 1.559 s against 1.563 s on the same question, while
    // Kissat's raw strength is worth five times: 0.31 s on that question and
    // 34.2 s against 167.9 s on the next one up.
    static const std::vector<std::string> order = {"kissat", "cryptominisat", "cadical"};
    return order;
}

SatSolver find_sat_solver(bool prefer_xor, const std::string& named,
                          const std::vector<std::string>& order) {
    const auto describe = [](const std::string& name, const std::string& path) {
        SatSolver solver;
        solver.found = !path.empty();
        solver.name = name;
        solver.path = path;
        solver.native_xor = (name == "cryptominisat");
        solver.writes_proofs = (name == "kissat");
        solver.finds_only = finds_only_by_name(name);
        return solver;
    };

    if (!named.empty()) {
        // A name with a slash in it is a path. The local search solvers are
        // built into a tree rather than installed, so `PATH` names none of
        // them, and the class still follows the binary's name.
        if (named.find('/') == std::string::npos) return describe(named, on_path(named));
        const std::filesystem::path binary(named);
        std::error_code ignored;
        const bool usable = std::filesystem::is_regular_file(binary, ignored);
        return describe(binary.filename().string(), usable ? named : "");
    }

    // `prefer_xor` is kept because the encoding still has to know whether to
    // write `x` lines, but it no longer decides which solver runs: the order
    // does, and `default_solver_order` records the measurement behind it.
    static_cast<void>(prefer_xor);
    const std::vector<std::string>& asked = order.empty() ? default_solver_order() : order;
    for (const std::string& name : asked) {
        const std::string path = on_path(name);
        if (!path.empty()) return describe(name, path);
    }
    return SatSolver();
}

std::string find_smt_solver() { return on_path("cvc5"); }

std::string find_proof_checker() { return on_path("drat-trim"); }

SolverRun run_solver(const linear_algebra::Cnf& formula, const SatSolver& solver,
                std::size_t memory_megabytes, std::size_t timeout_seconds,
                const std::string& proof_path, Tuning tuning, std::size_t seed) {
    SolverRun run;
    if (!solver.found) return run;
    run.solver_found = true;
    run.solver_name = solver.name;

    // A solver that can only find has no refutation for a proof to be of, which
    // is a different refusal from the one below: not that it cannot write the
    // file, but that it never has the verdict the file would certify.
    if (!proof_path.empty() && solver.finds_only) {
        throw std::invalid_argument(
            solver.name + " can only find: it never refutes, so there is no refutation for "
            "--proof to write. Ask kissat for the lower bound");
    }
    // A proof nobody wrote is worse than no proof asked for: the run answers no,
    // `proof_bytes` stays 0, and the caller reports a refusal that reads as
    // checked because a proof was requested. Only kissat takes a proof file here,
    // so anything else is told rather than quietly handed a dropped argument.
    if (!proof_path.empty() && !solver.writes_proofs) {
        throw std::invalid_argument(
            solver.name + " writes no DRAT proof here, so --proof would produce nothing and the "
            "refusal would rest on the solver's word after all. kissat takes a proof file as its "
            "second argument; install it, or ask the question without a proof");
    }

    const ScratchFile file(scratch_file(".cnf"));
    {
        std::ofstream out(file.path());
        linear_algebra::write_dimacs(out, formula, solver.native_xor);
    }

    // Only kissat has these, and passing them to anything else makes it print
    // usage and exit, which would read here as a solver that answered nothing.
    std::string configuration;
    if (solver.name == "kissat") {
        // No leading space. These used to be pasted into a shell line, where the
        // shell split them; they are an argv element now, and " --unsat" is a
        // filename kissat cannot read rather than a configuration it honours.
        if (tuning == Tuning::Satisfiable) configuration = "--sat";
        if (tuning == Tuning::Unsatisfiable) configuration = "--unsat";
    }

    const std::vector<std::string> command =
        solver.finds_only
            ? local_search_command(solver.name, solver.path, file.string(), seed, timeout_seconds)
            : solver_command(solver.path, file.string(), proof_path, configuration);

    const auto started = std::chrono::steady_clock::now();
    const std::string output = run_capped(command, memory_megabytes, timeout_seconds);
    run.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

    std::istringstream lines(output);
    run.model = linear_algebra::read_dimacs_model(lines);
    run.answered = run.model.answered;
    run.satisfiable = run.model.satisfiable;

    // A solver that can only find has said nothing when it says no. yalsat
    // prints `s UNSATISFIABLE` when unit propagation alone closes a formula, and
    // that may well be right; it is still not this class's to assert, because
    // the class exists so that its no is never read as a bound.
    if (solver.finds_only && run.answered && !run.satisfiable) run.answered = false;

    std::error_code ignored;
    if (!proof_path.empty() && run.answered && !run.satisfiable) {
        run.proof_bytes = static_cast<std::size_t>(std::filesystem::file_size(proof_path, ignored));
        run.proof = Proof::Written;

        // Checked by a program sharing no code with the solver that wrote it,
        // which is the whole point: otherwise a refusal is the solver's word.
        const std::string checker = find_proof_checker();
        if (!checker.empty()) {
            // No memory cap: the checker reads a proof rather than searching.
            const std::string verdict =
                run_capped({checker, file.string(), proof_path}, 0, timeout_seconds);
            run.proof = verdict.find("s VERIFIED") != std::string::npos ? Proof::Verified
                                                                       : Proof::Refuted;
        }
    }
    return run;
}

SolverRun run_smt_solver(const linear_algebra::SmtProblem& problem, std::size_t memory_megabytes,
                         std::size_t timeout_seconds) {
    SolverRun run;
    const std::string solver = find_smt_solver();
    if (solver.empty()) return run;
    run.solver_found = true;
    run.solver_name = "cvc5";

    const ScratchFile file(scratch_file(".smt2"));
    {
        std::ofstream out(file.path());
        linear_algebra::write_smtlib(out, problem);
    }

    const auto started = std::chrono::steady_clock::now();
    const std::string output =
        run_capped(solver_command(solver, file.string()), memory_megabytes, timeout_seconds);
    run.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

    std::istringstream lines(output);
    run.field_model = linear_algebra::read_smtlib_model(lines);
    run.answered = run.field_model.answered;
    run.satisfiable = run.field_model.satisfiable;
    return run;
}

}  // namespace satisfiability
