#include "solver_chain.h"

#include <unistd.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <sstream>

#include "branch_and_bound.h"
#include "external_solver.h"

namespace integer_programme {

namespace {

const char* binary_of(Backend backend) {
    switch (backend) {
        case Backend::Gurobi: return "gurobi_cl";
        case Backend::Cbc: return "cbc";
        case Backend::Glpk: return "glpsol";
        case Backend::LpSolve: return "lp_solve";
        case Backend::BuiltIn: return "";
    }
    return "";
}

}  // namespace

namespace {
unsigned& time_limit_storage() {
    static unsigned seconds = 300;
    return seconds;
}
}  // namespace

unsigned solver_time_limit() { return time_limit_storage(); }
void set_solver_time_limit(unsigned seconds) { time_limit_storage() = seconds; }

namespace {
std::vector<Backend>& ranking_storage() {
    static std::vector<Backend> ranking{Backend::Gurobi, Backend::Cbc, Backend::Glpk,
                                        Backend::LpSolve, Backend::BuiltIn};
    return ranking;
}
}  // namespace

const std::vector<Backend>& ranked_backends() { return ranking_storage(); }

std::string name_of(Backend backend) {
    switch (backend) {
        case Backend::Gurobi: return "gurobi";
        case Backend::Cbc: return "cbc";
        case Backend::Glpk: return "glpk";
        case Backend::LpSolve: return "lp_solve";
        case Backend::BuiltIn: return "built-in";
    }
    return "built-in";
}

Backend backend_named(const std::string& name, bool& recognised) {
    recognised = true;
    for (const Backend backend : ranked_backends()) {
        if (name_of(backend) == name) return backend;
    }
    recognised = false;
    return Backend::BuiltIn;
}

bool set_backend_order(const std::vector<std::string>& names, std::string& unrecognised) {
    if (names.empty()) return true;
    std::vector<Backend> order;
    order.reserve(names.size());
    for (const std::string& name : names) {
        bool recognised = false;
        const Backend backend = backend_named(name, recognised);
        if (!recognised) {
            unrecognised = name;
            return false;
        }
        order.push_back(backend);
    }
    ranking_storage() = order;
    return true;
}

namespace {

/// `PATH` walked here rather than asked of a shell.
///
/// `std::system("command -v cbc")` forked a shell per backend per process even on
/// a machine with nothing installed, which is four shells to learn nothing, and a
/// fork this module cannot see is exactly what the leak was about.
bool on_path(const char* binary) {
    if (*binary == '\0') return false;
    const char* search = std::getenv("PATH");
    if (search == nullptr) return false;

    std::istringstream directories(search);
    for (std::string directory; std::getline(directories, directory, ':');) {
        if (directory.empty()) directory = ".";
        const std::filesystem::path candidate = std::filesystem::path(directory) / binary;
        if (::access(candidate.c_str(), X_OK) == 0) return true;
    }
    return false;
}

}  // namespace

bool is_available(Backend backend) {
    if (backend == Backend::BuiltIn) return true;
    // One probe per backend per run: the answer cannot change under us, and the
    // chain asks the question once per programme solved.
    static std::array<char, 5> known{};
    const std::size_t slot = static_cast<std::size_t>(backend);
    if (known[slot] == 0) known[slot] = on_path(binary_of(backend)) ? 1 : 2;
    return known[slot] == 1;
}

std::vector<Backend> available_backends() {
    std::vector<Backend> present;
    for (const Backend backend : ranked_backends()) {
        if (is_available(backend)) present.push_back(backend);
    }
    return present;
}

Solution solve_with(Backend backend, const IntegerProgramme& programme, std::size_t node_limit) {
    if (backend == Backend::BuiltIn) return branch_and_bound(programme, node_limit);
    return run_backend(backend, programme);
}

Solution solve(const IntegerProgramme& programme, std::size_t node_limit) {
    for (const Backend backend : ranked_backends()) {
        if (backend == Backend::BuiltIn || !is_available(backend)) continue;
        const Solution answer = solve_with(backend, programme, node_limit);
        // A point can be checked and is. Anything else this backend might say,
        // infeasibility included, is one more decline to answer.
        if (answer.status == Status::Optimal) return answer;
    }
    return branch_and_bound(programme, node_limit);
}

}  // namespace integer_programme
