#include "local_search_solver.h"

#include <stdexcept>

namespace satisfiability {

bool finds_only_by_name(const std::string& name) {
    return name == "yalsat" || name == "probSAT" || name == "multilinear-sat";
}

std::vector<std::string> local_search_command(const std::string& name, const std::string& binary,
                                              const std::string& file, std::size_t seed,
                                              std::size_t timeout_seconds) {
    const std::string seed_text = std::to_string(seed);
    if (name == "yalsat") return {binary, file, seed_text};
    // `-a` prints the assignment, which probSAT otherwise keeps to itself.
    if (name == "probSAT") return {binary, "-a", file, seed_text};
    if (name == "multilinear-sat") {
        return {binary,   file,   "--time-limit", std::to_string(timeout_seconds),
                "--seed", seed_text, "--backend", "cpu"};
    }
    throw std::invalid_argument(name + " is not a solver this module knows how to run");
}

}  // namespace satisfiability
