/// Which integer programming solvers this machine has, best first.
#include <iostream>
#include <string>

#include "arguments.h"
#include "exit_code.h"
#include "solver_chain.h"
#include "tunables.h"

namespace {

/// The order the chain would really walk, and not the compiled one.
///
/// This command's whole output is the ranking, so reading `tunables.conf` here
/// is not decoration: printing the compiled order while `ilp_backend_order` had
/// moved it would make this the one place a caller looks that lies.
int run() {
    std::string unrecognised;
    if (!optimisation::set_backend_order(cli::tunables().ilp_backend_order, unrecognised)) {
        throw cli::ArgumentError("tunables.conf ilp_backend_order: no backend is called '" +
                                 unrecognised + "'");
    }

    std::cout << "integer programming backends, in preference order:\n";
    for (const optimisation::Backend backend : optimisation::ranked_backends()) {
        std::cout << "  " << (optimisation::is_available(backend) ? "present" : "absent ") << "  "
                  << optimisation::name_of(backend) << "\n";
    }
    std::cout << "\nThe first present backend answers; the built-in is always present and is the\n"
                 "only one whose answer needs no checking, so it is also the only one that may\n"
                 "report a problem infeasible.\n";
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main() {
    try {
        return run();
    } catch (const cli::ArgumentError& problem) {
        // A line of tunables.conf that could not be read. Nothing was listed, so
        // Usage rather than Error: the run never started.
        std::cerr << "list-solvers: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Usage);
    }
}
