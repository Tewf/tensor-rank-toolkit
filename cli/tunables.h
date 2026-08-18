#pragma once

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "arguments.h"

/// The numbers a run is bounded by, in a file instead of in the code.
///
/// They were spread through the tree in copies: `2048` megabytes and `300`
/// seconds in three places each, `5'000'000` nodes in two, `200'000` states in
/// two more, and two solver preference orders written as literals inside the
/// functions that walk them. A copy is not a tunable. Changing the SAT timeout
/// meant finding three places and hoping there were three, and the plateau
/// state cap could not be changed at all: no flag reaches it, so the only way to
/// move it was to edit `minimise_rank_main.cpp` and rebuild.
///
/// Every default here is the number that was compiled in before, so a machine
/// with no file behaves exactly as it did. That is deliberate: a config file
/// that changes the published numbers by existing would make every table in the
/// README depend on a file that is not in it.
///
/// `tunables.conf` at the root of the repository is the one shipped file, and it
/// writes out every default, so the format needs no documentation apart from
/// itself.
namespace cli {

/// The environment variable that names a file, for a run that wants its own.
inline constexpr const char* tunables_variable = "BILINEAR_TUNABLES";

/// The file looked for in the working directory when it does not.
inline constexpr const char* tunables_filename = "tunables.conf";

/// What a run may spend, and which solver it asks first.
struct Tunables {
    /// Nodes an exhaustive search may visit before it gives up, which is a
    /// budget and never a refutation. `exhaustive_search.h`, `decide-rank`.
    std::size_t search_node_limit = 5'000'000;

    /// Nodes the built-in branch and bound may open. `decide-rank-by-ilp`,
    /// `curve_bounds`.
    std::size_t ilp_node_limit = 200'000;

    /// Distinct subspaces a plateau crossing may visit. PROVISIONAL: never
    /// measured, and until now unreachable from the command line, so no run has
    /// ever tried another value. `flip_graph/plateau_search.h`.
    std::size_t plateau_state_budget = 200'000;

    /// What a SAT solver may take before it is killed, per question.
    /// `satisfiability/solver_process.h`, `rank_question.h`.
    std::size_t sat_memory_megabytes = 2048;
    std::size_t sat_timeout_seconds = 300;

    /// What an outside integer programme solver may take, per programme.
    /// `integer_programme/solver_chain.h`.
    std::size_t ilp_time_limit_seconds = 300;

    /// Which SAT solver is asked first, of those on `PATH`. kissat leads
    /// because it is the strongest on unsatisfiable instances, and an
    /// unsatisfiable instance is where a lower bound lives.
    std::vector<std::string> sat_solver_order{"kissat", "cryptominisat", "cadical"};

    /// Which integer programme backend is asked first, of those on `PATH`. The
    /// built-in trails because it is the slowest, and it is also the only one
    /// whose `infeasible` is believed without being checked against the model.
    std::vector<std::string> ilp_backend_order{"gurobi", "cbc", "glpk", "lp_solve", "built-in"};
};

/// The count-valued tunables, under the names a file spells them with.
inline const std::vector<std::pair<std::string, std::size_t Tunables::*>>& counted_tunables() {
    static const std::vector<std::pair<std::string, std::size_t Tunables::*>> table{
        {"search_node_limit", &Tunables::search_node_limit},
        {"ilp_node_limit", &Tunables::ilp_node_limit},
        {"plateau_state_budget", &Tunables::plateau_state_budget},
        {"sat_memory_megabytes", &Tunables::sat_memory_megabytes},
        {"sat_timeout_seconds", &Tunables::sat_timeout_seconds},
        {"ilp_time_limit_seconds", &Tunables::ilp_time_limit_seconds},
    };
    return table;
}

/// The list-valued ones, whose value is names separated by spaces.
inline const std::vector<std::pair<std::string, std::vector<std::string> Tunables::*>>&
listed_tunables() {
    static const std::vector<std::pair<std::string, std::vector<std::string> Tunables::*>> table{
        {"sat_solver_order", &Tunables::sat_solver_order},
        {"ilp_backend_order", &Tunables::ilp_backend_order},
    };
    return table;
}

inline std::string trimmed(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) return "";
    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

inline std::vector<std::string> words_of(const std::string& text) {
    std::istringstream stream(text);
    std::vector<std::string> words;
    for (std::string word; stream >> word;) words.push_back(word);
    return words;
}

/// One `name = value` line, where `where` is the file and line it came from so a
/// refusal points at it.
///
/// A name nothing here has is refused rather than ignored. A typo that is
/// ignored is the worst outcome available: the run is bounded by a number the
/// file appears to have changed and did not.
inline void set_tunable(Tunables& values, const std::string& name, const std::string& text,
                        const std::string& where) {
    for (const auto& [key, field] : counted_tunables()) {
        if (key == name) {
            values.*field = parse_count(where + " " + name, text);
            return;
        }
    }
    for (const auto& [key, field] : listed_tunables()) {
        if (key != name) continue;
        values.*field = words_of(text);
        if ((values.*field).empty()) {
            throw ArgumentError(where + " " + name + " expects at least one name");
        }
        return;
    }
    throw ArgumentError(where + ": no tunable is called '" + name + "'");
}

/// `name = value` lines. `#` starts a comment, anywhere on a line, and a blank
/// line is nothing: the file is commented the same way the commands comment
/// their own output, so one convention covers both.
inline void read_tunables_from(std::istream& text, const std::string& path, Tunables& values) {
    std::string line;
    for (std::size_t number = 1; std::getline(text, line); ++number) {
        const std::string content = trimmed(line.substr(0, line.find('#')));
        if (content.empty()) continue;

        const std::size_t split = content.find('=');
        const std::string where = path + ":" + std::to_string(number);
        if (split == std::string::npos) {
            throw ArgumentError(where + ": '" + content + "' is not a 'name = value' line");
        }
        set_tunable(values, trimmed(content.substr(0, split)),
                    trimmed(content.substr(split + 1)), where);
    }
}

/// The file named by the environment if there is one, else `tunables.conf` in
/// the working directory, else nothing.
///
/// A file named in the environment and missing is an error, because it was asked
/// for by name. A `tunables.conf` that is simply absent is not, because the
/// defaults are the same numbers it would have held.
inline Tunables tunables_from_file() {
    Tunables values;
    if (const char* named = std::getenv(tunables_variable)) {
        std::ifstream file(named);
        if (!file) {
            throw ArgumentError(std::string(tunables_variable) + " names '" + named +
                                "', which cannot be read");
        }
        read_tunables_from(file, named, values);
        return values;
    }
    std::ifstream file(tunables_filename);
    if (file) read_tunables_from(file, tunables_filename, values);
    return values;
}

/// The tunables this run uses, read the first time they are asked for and not
/// again. A command that reads the file twice could be bounded by two different
/// numbers in one run, which is the sort of thing that is discovered a week
/// later in a table.
inline const Tunables& tunables() {
    static const Tunables values = tunables_from_file();
    return values;
}

}  // namespace cli
