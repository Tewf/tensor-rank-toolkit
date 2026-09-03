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
#include "machine.h"

/// The numbers a run is bounded by, in a file instead of in the code.
///
/// They were spread through the tree in copies: `2048` megabytes and `300`
/// seconds in three places each, `5'000'000` nodes in two, `200'000` states in
/// two more, and two solver preference orders written as literals inside the
/// functions that walk them. A copy is not a tunable. Changing the SAT timeout
/// meant finding three places and hoping there were three, and the plateau
/// state cap could not be changed at all: no flag reached it, so the only way to
/// move it was to edit `minimise_rank_main.cpp` and rebuild. It now has
/// `--plateau-states`, and every other field here reaches the code it names.
///
/// **Precedence, strongest first: an explicit flag, then this file, then the
/// compiled default.** A command reads these into its settings before it walks
/// its arguments, so a flag that was given always overwrites what the file said,
/// and a flag that was not leaves it standing. The whole table, per tool, is
/// [`OPTIONS.md`](../OPTIONS.md).
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
///
/// Each field names the library setting it fills in and the commands that fill
/// it. **The command reads the file; the library never does.** A header that
/// opened `tunables.conf` would bind every caller to a working directory, so
/// each of these is passed down as the settings or budget argument the library
/// already takes, and the literal beside that argument stays as the compiled
/// default a caller who passes nothing still gets.
struct Tunables {
    /// Nodes an exhaustive search may visit before it gives up, which is a
    /// budget and never a refutation. Fills `SearchBudget::node_limit`
    /// (`methods/bilinear_rank/exhaustive/exhaustive_search.h`) and `StrictSettings::node_limit`
    /// (`methods/bilinear_rank/canonical_augmentation/strict_deflation.h`), from `decide-rank
    /// --node-limit` and `deflate-strictly --node-limit`.
    std::size_t search_node_limit = 5'000'000;

    /// Elements one leaf of an exhaustive search may examine before it gives up
    /// on that leaf, which is likewise a budget and never a refutation. Fills
    /// `SearchBudget::leaf_element_limit`
    /// (`methods/bilinear_rank/exhaustive/exhaustive_search.h`), from `decide-rank
    /// --leaf-limit`. The node limit bounds how many leaves are reached and
    /// nothing inside one, which is what this is for.
    std::size_t search_leaf_limit = 100'000'000;

    /// Nodes the built-in branch and bound may open. Fills
    /// `curve_bounds::set_solver_node_limit`
    /// (`curve_bounds/interpolation_by_solver.h`), from `curve-bounds
    /// --node-limit`. It bounds the built-in only, whichever route asks.
    std::size_t ilp_node_limit = 200'000;

    /// Distinct subspaces a plateau crossing may visit. Measured: <2,2,2>
    /// crosses to 7 at 380 and stays at 8 at 370, published in
    /// `methods/bilinear_rank/flip_graph/results.json`. Why the default is not 380 is in
    /// `tunables.conf` beside the value. Fills `cross_plateaus`'s
    /// `state_budget` (`methods/bilinear_rank/flip_graph/plateau_search.h`), from
    /// `minimise-rank --plateau-states`.
    std::size_t plateau_state_budget = 200'000;

    /// What a SAT solver may take before it is killed, per question. **Derived
    /// from the machine**, an eighth of what it has, which is the 2048 this
    /// repository has always used on a 16 GB laptop; `auto` in the file asks for
    /// the same derivation and a number pins one.
    /// `satisfiability/rank_question.cpp` then divides it by the worker count,
    /// so the dividend is the machine's rather than this chassis's. Fills
    /// `SolveOptions::memory_megabytes` and `::timeout_seconds`
    /// (`satisfiability/rank_question.h`), which `run_solver`
    /// (`satisfiability/solver_process.h`) is handed, from `decide-rank-by-sat
    /// --max-memory` and `--timeout`, and from the same two flags on
    /// `deflate-strictly`.
    std::size_t sat_memory_megabytes = run_limits::suggested_memory_budget() >> 20;
    std::size_t sat_timeout_seconds = 300;

    /// What an outside integer programme solver may take, per programme. Fills
    /// `integer_programme::set_solver_time_limit`
    /// (`integer_programme/solver_chain.h`), from `curve-bounds
    /// --solver-timeout`. It does not reach the built-in, which has no clock.
    std::size_t ilp_time_limit_seconds = 300;

    /// Elements below which a bulk question stays on the host whatever
    /// `device_order` says, because a launch costs more than the work. Fills
    /// `run_limits::set_launch_floor` (`infrastructure/run_limits/device.h`), from `decide-rank
    /// --device`, which sets it to zero when a run asks for the card outright.
    /// Measured 2026-08-21 by `measure-leaf floor`; the table of crossovers and
    /// what was swept against what is in `infrastructure/run_limits/device.cpp`.
    std::size_t device_launch_floor{8192};

    /// Which SAT solver is asked first, of those on `PATH`. kissat leads
    /// because it is the strongest on unsatisfiable instances, and an
    /// unsatisfiable instance is where a lower bound lives. Fills
    /// `SolveOptions::solver_order`, which `find_sat_solver`
    /// (`satisfiability/solver_process.h`) walks. `--solver <name>` pins one
    /// outright and so overrides the order rather than reordering it.
    std::vector<std::string> sat_solver_order{"kissat", "cryptominisat", "cadical"};

    /// Which integer programme backend is asked first, of those on `PATH`. The
    /// built-in trails because it is the slowest, and it is also the only one
    /// whose `infeasible` is believed without being checked against the model.
    /// Fills `integer_programme::set_backend_order`
    /// (`integer_programme/solver_chain.h`), from `curve-bounds`, whose
    /// `--solvers` prints the order that would actually run.
    std::vector<std::string> ilp_backend_order{"gurobi", "cbc", "glpk", "lp_solve", "built-in"};

    /// Which processor answers a bulk question first, of those this build can
    /// reach. Fills `run_limits::set_device_order` (`infrastructure/run_limits/device.h`),
    /// which `decide-rank` then overwrites from the plan it chose, so this is
    /// the order the *rule* starts from and `--device` is what settles it. A
    /// build without `nvcc` has no backend behind `gpu` and resolves to the
    /// host, which the plan's device line says rather than leaving a reader to
    /// wonder.
    std::vector<std::string> device_order{"gpu", "cpu"};
};

/// The count-valued tunables, under the names a file spells them with.
inline const std::vector<std::pair<std::string, std::size_t Tunables::*>>& counted_tunables() {
    static const std::vector<std::pair<std::string, std::size_t Tunables::*>> table{
        {"search_node_limit", &Tunables::search_node_limit},
        {"search_leaf_limit", &Tunables::search_leaf_limit},
        {"ilp_node_limit", &Tunables::ilp_node_limit},
        {"plateau_state_budget", &Tunables::plateau_state_budget},
        {"sat_memory_megabytes", &Tunables::sat_memory_megabytes},
        {"sat_timeout_seconds", &Tunables::sat_timeout_seconds},
        {"ilp_time_limit_seconds", &Tunables::ilp_time_limit_seconds},
        {"device_launch_floor", &Tunables::device_launch_floor},
    };
    return table;
}

/// The counts a file may spell `auto`, and what `auto` resolves to.
///
/// **`auto` is a machine reading, never a measurement.** A tunable belongs here
/// only when its right value is a property of the hardware the binary is on, so
/// that `fitted-or-genuine.md`'s fitted column can shrink without a person
/// re-measuring anything. `device_launch_floor` is deliberately not here: its
/// right value is a ratio of two timings and the only honest way to get one is
/// `measure-leaf floor`, which takes a stopwatch to the machine and so cannot
/// run inside every command's start-up.
inline const std::vector<std::pair<std::string, std::size_t (*)()>>& machine_read_tunables() {
    static const std::vector<std::pair<std::string, std::size_t (*)()>> table{
        {"sat_memory_megabytes", [] { return run_limits::suggested_memory_budget() >> 20; }},
    };
    return table;
}

/// The list-valued ones, whose value is names separated by spaces.
inline const std::vector<std::pair<std::string, std::vector<std::string> Tunables::*>>&
listed_tunables() {
    static const std::vector<std::pair<std::string, std::vector<std::string> Tunables::*>> table{
        {"sat_solver_order", &Tunables::sat_solver_order},
        {"ilp_backend_order", &Tunables::ilp_backend_order},
        {"device_order", &Tunables::device_order},
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
        if (key != name) continue;
        if (text != "auto") {
            values.*field = parse_count(where + " " + name, text);
            return;
        }
        for (const auto& [named, resolve] : machine_read_tunables()) {
            if (named != name) continue;
            values.*field = resolve();
            return;
        }
        throw ArgumentError(where + " " + name +
                            " has no machine reading behind it, so 'auto' means nothing here; "
                            "give it a number");
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
