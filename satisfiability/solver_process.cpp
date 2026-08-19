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
    const std::filesystem::path log = scratch_file(".log");
    const bool started = bilinear_rank::run_to_completion(
        command, log, bilinear_rank::ChildLimits{static_cast<double>(seconds), megabytes, false});

    std::string captured;
    if (started) {
        std::ifstream reading(log);
        std::ostringstream all;
        all << reading.rdbuf();
        captured = all.str();
    }
    std::error_code ignored;
    std::filesystem::remove(log, ignored);
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
        return solver;
    };

    if (!named.empty()) return describe(named, on_path(named));

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
                const std::string& proof_path, Tuning tuning) {
    SolverRun run;
    if (!solver.found) return run;
    run.solver_found = true;
    run.solver_name = solver.name;

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

    const std::filesystem::path file = scratch_file(".cnf");
    {
        std::ofstream out(file);
        linear_algebra::write_dimacs(out, formula, solver.native_xor);
    }

    // Only kissat has these, and passing them to anything else makes it print
    // usage and exit, which would read here as a solver that answered nothing.
    std::string configuration;
    if (solver.name == "kissat") {
        if (tuning == Tuning::Satisfiable) configuration = " --sat";
        if (tuning == Tuning::Unsatisfiable) configuration = " --unsat";
    }

    const auto started = std::chrono::steady_clock::now();
    const std::string output =
        run_capped(solver_command(solver.path, file.string(), proof_path, configuration),
                   memory_megabytes, timeout_seconds);
    run.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

    std::istringstream lines(output);
    run.model = linear_algebra::read_dimacs_model(lines);
    run.answered = run.model.answered;
    run.satisfiable = run.model.satisfiable;

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
    std::filesystem::remove(file, ignored);
    return run;
}

SolverRun run_smt_solver(const linear_algebra::SmtProblem& problem, std::size_t memory_megabytes,
                         std::size_t timeout_seconds) {
    SolverRun run;
    const std::string solver = find_smt_solver();
    if (solver.empty()) return run;
    run.solver_found = true;
    run.solver_name = "cvc5";

    const std::filesystem::path file = scratch_file(".smt2");
    {
        std::ofstream out(file);
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

    std::error_code ignored;
    std::filesystem::remove(file, ignored);
    return run;
}

}  // namespace satisfiability
