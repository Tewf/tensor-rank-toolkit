#include "external_solver.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "mps_format.h"
#include "whole_numbers.h"

namespace optimisation {

namespace {

std::filesystem::path scratch_path(const char* suffix) {
    return std::filesystem::temp_directory_path() /
           ("optimisation-" + std::to_string(::getpid()) + suffix);
}

std::vector<std::string> lines_of(const std::filesystem::path& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    for (std::string line; std::getline(file, line);) lines.push_back(line);
    return lines;
}

std::vector<std::string> words_of(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> words;
    for (std::string word; stream >> word;) words.push_back(word);
    return words;
}

/// An exact rational from whatever decimal a solver printed, exponent included:
/// lp_solve writes `1e+30` for an absent bound and CBC pads to eight places.
Number number_from(const std::string& text) {
    std::size_t position = 0;
    bool negative = false;
    if (position < text.size() && (text[position] == '+' || text[position] == '-')) {
        negative = text[position] == '-';
        ++position;
    }
    const auto digit_here = [&] {
        return position < text.size() && std::isdigit(static_cast<unsigned char>(text[position]));
    };

    Givaro::Integer digits = 0;
    long exponent = 0;
    for (; digit_here(); ++position) digits = digits * 10 + (text[position] - '0');
    if (position < text.size() && text[position] == '.') {
        for (++position; digit_here(); ++position) {
            digits = digits * 10 + (text[position] - '0');
            --exponent;
        }
    }
    if (position < text.size() && (text[position] == 'e' || text[position] == 'E')) {
        exponent += std::strtol(text.c_str() + position + 1, nullptr, 10);
    }

    Givaro::Integer power = 1;
    for (long step = 0; step < (exponent < 0 ? -exponent : exponent); ++step) power *= 10;
    const Number value = exponent < 0 ? Number(digits) / Number(power) : Number(digits) * Number(power);
    return negative ? Number(0) - value : value;
}

/// Values by the names the writer gave the columns. lp_solve, CBC and Gurobi all
/// put `name value` somewhere on the line, whatever else is on it, so the rule is
/// "a word that names a column, then the word after it". The first occurrence
/// wins, which is what keeps lp_solve's later sensitivity table from overwriting
/// the answer with its own numbers.
bool read_by_name(const std::vector<std::string>& lines, std::vector<Number>& values) {
    std::map<std::string, std::size_t> column_of;
    for (std::size_t column = 0; column < values.size(); ++column) {
        column_of[column_name(column)] = column;
    }

    std::vector<bool> seen(values.size(), false);
    for (const std::string& line : lines) {
        const std::vector<std::string> words = words_of(line);
        for (std::size_t position = 0; position + 1 < words.size(); ++position) {
            const auto found = column_of.find(words[position]);
            if (found == column_of.end() || seen[found->second]) continue;
            values[found->second] = number_from(words[position + 1]);
            seen[found->second] = true;
        }
    }
    return std::find(seen.begin(), seen.end(), false) == seen.end();
}

/// GLPK's plain format names nothing: `j <column> <value>` for a mixed integer
/// answer, and `j <column> <status> <primal> <dual>` when the programme turned
/// out to have no integer variables at all.
bool read_by_index(const std::vector<std::string>& lines, std::vector<Number>& values) {
    std::vector<bool> seen(values.size(), false);
    for (const std::string& line : lines) {
        const std::vector<std::string> words = words_of(line);
        if (words.size() < 3 || words[0] != "j") continue;
        const std::size_t column = std::strtoul(words[1].c_str(), nullptr, 10) - 1;
        if (column >= values.size()) continue;
        values[column] = number_from(words.size() >= 4 ? words[3] : words[2]);
        seen[column] = true;
    }
    return std::find(seen.begin(), seen.end(), false) == seen.end();
}

struct Recipe {
    /// The binary and its arguments, unquoted, because no shell sees them.
    std::vector<std::string> command;
    bool answer_in_log = false;  // lp_solve prints its answer rather than filing it
    bool by_index = false;       // only GLPK
    const char* infeasible = "";
};

Recipe recipe_for(Backend backend, const std::filesystem::path& model,
                  const std::filesystem::path& answer) {
    const std::string model_path = model.string();
    const std::string answer_path = answer.string();
    switch (backend) {
        case Backend::Gurobi:
            return {{"gurobi_cl", "ResultFile=" + answer_path, model_path}, false, false,
                    "infeasible"};
        case Backend::Cbc:
            return {{"cbc", model_path, "-solve", "-solution", answer_path}, false, false,
                    "infeasible"};
        case Backend::Glpk:
            return {{"glpsol", "--mps", model_path, "-w", answer_path}, false, true,
                    "no primal feasible solution"};
        case Backend::LpSolve:
            return {{"lp_solve", "-mps", model_path, "-S3"}, true, false, "problem is infeasible"};
        case Backend::BuiltIn:
            break;
    }
    return {};
}

/// Start the solver in a process group of its own, wait out the clock, and kill
/// the **group** if the clock runs out. False when it could not be started.
///
/// This replaces `std::system` under a `timeout` prefix, which was not a handle
/// on anything. The prefix bounded the orphan's *duration* and not its effect:
/// for those seconds it held a core, and every measurement taken afterwards read
/// slow. It defeated three runs of the backend comparison, and one ILP cell read
/// 6.62 s against a stray where it reads 3.34 s clean.
///
/// The group is what must be killed, not the child. CBC and GLPK are single
/// processes today, but a backend that forks a worker leaves it behind when only
/// its parent is signalled, which is the same leak with an extra step. `setpgid`
/// in the child and `killpg` in the parent covers both, and it never matches on a
/// process name, so it cannot reach a solver somebody else is running or, worse,
/// the process doing the killing.
///
/// **The child also caps itself, and dropping that would have been a regression.**
/// A parent killed mid-solve cannot kill anything, and what the old `timeout`
/// prefix genuinely bought was that the orphan died within the cap rather than at
/// reboot. `alarm` is preserved across `execvp` while SIGALRM's disposition resets
/// to terminate, so the solver carries its own deadline and needs nobody alive to
/// enforce it. The parent's kill is what makes the *call* leave nothing running;
/// the alarm is what makes a *dead parent* leave nothing running. Both are wanted
/// and neither replaces the other.
bool run_to_completion(const std::vector<std::string>& command,
                       const std::filesystem::path& log) {
    std::vector<char*> argv;
    argv.reserve(command.size() + 1);
    for (const std::string& word : command) argv.push_back(const_cast<char*>(word.c_str()));
    argv.push_back(nullptr);

    const std::string log_path = log.string();
    const pid_t child = ::fork();
    if (child < 0) return false;

    if (child == 0) {
        // Its own group, set on both sides of the fork because whichever runs
        // first wins and neither ordering is guaranteed.
        ::setpgid(0, 0);
        const int null_in = ::open("/dev/null", O_RDONLY);
        const int output = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (null_in >= 0) ::dup2(null_in, STDIN_FILENO);
        if (output >= 0) {
            ::dup2(output, STDOUT_FILENO);
            ::dup2(output, STDERR_FILENO);
        }
        // One second of grace, so the parent's kill is what normally ends a run
        // and this only fires when there is no parent left to do it.
        ::alarm(solver_time_limit() + 1);
        ::execvp(argv[0], argv.data());
        // Only reached when the binary vanished between the availability probe
        // and here. `_exit` and not `exit`, so no parent's destructors run twice.
        ::_exit(127);
    }

    ::setpgid(child, child);

    // Polled rather than alarmed, because a SIGCHLD handler is process-wide state
    // for a library to be setting and this is called in a loop over backends.
    //
    // The interval doubles from 50 us to 20 ms. A flat interval has to choose
    // between adding it to every millisecond-long solve and burning syscalls for
    // five minutes on a long one; doubling pays neither. `test_optimisation` runs
    // a battery of tiny programmes through every installed backend, so the fast
    // end is not hypothetical.
    const double deadline_seconds = solver_time_limit();
    long pause_nanoseconds = 50'000;
    double waited = 0;
    int status = 0;
    while (waited < deadline_seconds) {
        const pid_t done = ::waitpid(child, &status, WNOHANG);
        if (done == child) return true;
        // EINTR is not an answer about the child, and returning here would walk
        // away from a solver still holding a core, which is the whole defect.
        if (done < 0 && errno == EINTR) continue;
        if (done < 0) {
            ::killpg(child, SIGKILL);
            ::waitpid(child, &status, 0);
            return false;
        }
        const timespec pause{0, pause_nanoseconds};
        ::nanosleep(&pause, nullptr);
        waited += static_cast<double>(pause_nanoseconds) / 1e9;
        if (pause_nanoseconds < 20'000'000) pause_nanoseconds *= 2;
    }

    ::killpg(child, SIGKILL);
    // Blocking, so the child is reaped rather than left a zombie, and the caller
    // returns to a machine with nothing of ours running on it.
    ::waitpid(child, &status, 0);
    return true;
}

bool log_says(const std::filesystem::path& log, const char* phrase) {
    if (*phrase == '\0') return false;
    std::string text;
    for (const std::string& line : lines_of(log)) text += line + "\n";
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char letter) { return std::tolower(letter); });
    return text.find(phrase) != std::string::npos;
}

}  // namespace

Solution run_backend(Backend backend, const IntegerProgramme& programme) {
    Solution solution;
    solution.solved_by = name_of(backend);
    if (backend == Backend::BuiltIn || !is_available(backend)) return solution;

    const std::filesystem::path model = scratch_path(".mps");
    const std::filesystem::path answer = scratch_path(".sol");
    const std::filesystem::path log = scratch_path(".log");
    std::filesystem::remove(answer);
    {
        std::ofstream file(model);
        file << mps_of(programme);
    }

    const Recipe recipe = recipe_for(backend, model, answer);
    // A solver that refuses the model exits non-zero and leaves nothing to read,
    // which the parse below finds out for itself, and one killed at the deadline
    // leaves a partial file the same parse rejects. Only a fork that failed is
    // worth short-circuiting on.
    const bool started = run_to_completion(recipe.command, log);

    std::vector<Number> values(programme.variables.size(), Number(0));
    const std::vector<std::string> output = lines_of(recipe.answer_in_log ? log : answer);
    const bool complete =
        started && (recipe.by_index ? read_by_index(output, values) : read_by_name(output, values));

    if (complete) {
        // A whole variable's decimal is a rendering of the integer it was, so it
        // is put back on that integer before the model is asked to accept it.
        for (std::size_t index = 0; index < values.size(); ++index) {
            if (programme.variables[index].integral) {
                values[index] = Number(nearest_whole(values[index]));
            }
        }
        if (satisfies(programme, values)) {
            solution.status = Status::Optimal;
            solution.values = values;
            solution.objective = objective_at(programme, values);
        }
    }
    if (solution.status != Status::Optimal && log_says(log, recipe.infeasible)) {
        solution.status = Status::Infeasible;
    }

    std::filesystem::remove(model);
    std::filesystem::remove(answer);
    std::filesystem::remove(log);
    return solution;
}

}  // namespace optimisation
