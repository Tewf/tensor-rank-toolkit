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

#include "child_process.h"
#include "mps_format.h"
#include "whole_numbers.h"

namespace integer_programme {

namespace {

std::filesystem::path scratch_path(const char* suffix) {
    return std::filesystem::temp_directory_path() /
           ("integer-programme-" + std::to_string(::getpid()) + suffix);
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
    // One launcher for the whole repository, in `run_limits`. No memory cap and
    // stderr in the log, which is exactly what this route has always run with.
    const bool started = run_limits::run_to_completion(
        recipe.command, log, run_limits::ChildLimits{static_cast<double>(solver_time_limit()), 0, true});

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

}  // namespace integer_programme
