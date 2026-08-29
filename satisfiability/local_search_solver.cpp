#include "local_search_solver.h"

#include <stdexcept>

namespace satisfiability {

bool finds_only_by_name(const std::string& name) {
    return name == "yalsat" || name == "xnfsat" || name == "probSAT" || name == "multilinear-sat";
}

bool reads_xor_lines_by_name(const std::string& name) { return name == "xnfsat"; }

std::vector<std::string> local_search_command(const std::string& name, const std::string& binary,
                                              const std::string& file, std::size_t seed,
                                              std::size_t timeout_seconds) {
    const std::string seed_text = std::to_string(seed);
    if (name == "yalsat") return {binary, file, seed_text};
    // xnfsat ships with the witness off where yalsat ships with it on, and a
    // `s SATISFIABLE` with no `v` line is a yes nothing can check: the
    // decomposition check would refuse it as a model that rebuilds nothing.
    if (name == "xnfsat") return {binary, "--witness=1", file, seed_text};
    // `-a` prints the assignment, which probSAT otherwise keeps to itself.
    if (name == "probSAT") return {binary, "-a", file, seed_text};
    if (name == "multilinear-sat") {
        // Its CPU backend is OpenMP over the batch and takes every core it can
        // see: measured at twelve threads and 350% of a core per process. The
        // protocol here is one core, so it is launched through `env` with the
        // thread count in the argument vector, where the recorded command shows
        // it, rather than by mutating this process's environment.
        return {"env",    "OMP_NUM_THREADS=1", binary, file, "--time-limit",
                std::to_string(timeout_seconds), "--seed", seed_text, "--backend", "cpu"};
    }
    throw std::invalid_argument(name + " is not a solver this module knows how to run");
}

}  // namespace satisfiability
