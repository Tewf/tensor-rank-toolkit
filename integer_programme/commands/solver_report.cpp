/// Which integer programming solvers this machine has, best first.
#include <string>

#include "arguments.h"
#include "exit_code.h"
#include "report.h"
#include "solver_chain.h"
#include "tunables.h"

namespace {

void usage() {
    cli::note() << "usage: list-solvers\n"
                   "       list-solvers --help\n"
                   "\n"
                   "  Prints the integer programming backends in preference order, and\n"
                   "  whether each is on PATH. It takes no settings: the order comes from\n"
                   "  ilp_backend_order in tunables.conf, which is why this reads the file.\n"
                   "  --help  print this and stop, as exit 2";
}

/// The order the chain would really walk, and not the compiled one.
///
/// This command's whole output is the ranking, so reading `tunables.conf` here
/// is not decoration: printing the compiled order while `ilp_backend_order` had
/// moved it would make this the one place a caller looks that lies.
int run(int argc, char** argv) {
    // Walked rather than ignored. `main` took no arguments at all, so every word
    // on the line was dropped in silence and `list-solvers --help` printed the
    // table and left as 0: a command that answered a question nobody asked.
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
        arguments.refuse();
    }
    if (!arguments.filename().empty()) {
        throw cli::ArgumentError("nothing here reads a file, and '" + arguments.filename() +
                                 "' is not a flag");
    }

    std::string unrecognised;
    if (!optimisation::set_backend_order(cli::tunables().ilp_backend_order, unrecognised)) {
        throw cli::ArgumentError("tunables.conf ilp_backend_order: no backend is called '" +
                                 unrecognised + "'");
    }

    cli::result() << "integer programming backends, in preference order:\n";
    for (const optimisation::Backend backend : optimisation::ranked_backends()) {
        cli::result() << "  " << (optimisation::is_available(backend) ? "present" : "absent ")
                      << "  " << optimisation::name_of(backend) << "\n";
    }
    cli::result() << "\nThe first present backend answers; the built-in is always present and is"
                     " the\nonly one whose answer needs no checking, so it is also the only one"
                     " that may\nreport a problem infeasible.\n";
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line, or a line of tunables.conf, that could not
        // be read. Nothing was listed, so Usage rather than Error: the run never
        // started.
        cli::note() << "list-solvers: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    }
}
