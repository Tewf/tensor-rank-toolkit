/// Step 3 of the Chudnovsky-Chudnovsky roadmap, from a command line.
///
/// The module had no entry point at all, so the only way to run it was to write
/// C++ against it. There is no input *file* because there is nothing to put in
/// one: the whole input is a divisor degree and a list of `degree:count` pairs,
/// which is step 2's output and is supplied rather than computed.
///
/// What the number means is [`../README.md`](../README.md), and it is weaker than
/// it looks: this reports the best the method could give **if** a curve with that
/// supply exists and admits an interpolation system, and neither is checked here.
#include <stdexcept>
#include <string>
#include <vector>

#include "arguments.h"
#include "exit_code.h"
#include "interpolation_by_solver.h"
#include "interpolation_programme.h"
#include "memory_budget.h"
#include "report.h"
#include "solver_chain.h"
#include "symmetric_bound_table.h"
#include "tunables.h"

namespace {

void usage() {
    cli::note() << "usage: curve-bounds --degree G --points d:n [--points d:n ...]\n"
                   "       curve-bounds --table\n"
                   "       curve-bounds --solvers\n"
                   "       curve-bounds --help\n"
                   "\n"
                   "  --degree G      the divisor's degree, spent exactly and not as a budget.\n"
                   "                  Every point costs something, so minimising over smaller\n"
                   "                  divisors too would always answer 'one rational point,\n"
                   "                  cost 1', which is a bound on nothing.\n"
                   "  --points d:n    n distinct closed points of degree d are available. Repeat\n"
                   "                  for each degree. This is step 2's output, and step 2 is\n"
                   "                  not in this repository.\n"
                   "  --table         print [rambaud2014, Table 1] as transcribed, and stop\n"
                   "  --solvers       print the integer programming backends in preference\n"
                   "                  order, and whether each is on PATH, and stop. The order\n"
                   "                  is ilp_backend_order in tunables.conf, which is why this\n"
                   "                  reads the file rather than printing the compiled one.\n"
                   "                  Was the separate command `list-solvers`\n"
                   "  --route built-in|chain|enumeration\n"
                   "                  which minimiser answers. built-in is the default: exact\n"
                   "                  branch and bound in rationals, whose optimum is a proof,\n"
                   "                  and the fastest of the three at every size measured.\n"
                   "                  chain asks the first installed MILP backend, whose point\n"
                   "                  the model accepts but whose optimality it cannot check.\n"
                   "                  enumeration is the dynamic programme, exact and the\n"
                   "                  cross-check.\n"
                   "  --node-limit N  nodes the built-in branch and bound may open, from\n"
                   "                  ilp_node_limit in tunables.conf when this is not given.\n"
                   "                  Reaching it falls back to the dynamic programme\n"
                   "  --solver-timeout N\n"
                   "                  seconds an outside solver may run, from\n"
                   "                  ilp_time_limit_seconds in tunables.conf when this is not\n"
                   "                  given. --route chain only: the built-in has no clock\n"
                   "  --max-memory N  bytes one bulk allocation may take, 2G by default. The\n"
                   "                  frontier is quadratic in --degree, so this is the flag\n"
                   "                  that refuses a degree this machine cannot hold\n"
                   "  --help          print this and stop, as exit 2\n"
                   "\n"
                   "  The result is an envelope, not a bound on mu_sym_q(m): steps 2 and 4 of\n"
                   "  the roadmap are absent, so nothing here checks that such a curve exists.\n";
}

/// `d:n`, refused rather than guessed at. `std::stoull` reads a flag as zero and
/// carries on, which is how `--points --degree` becomes a supply.
curve_bounds::PointSupply parse_supply(const std::string& text) {
    const std::size_t colon = text.find(':');
    if (colon == std::string::npos) {
        throw std::runtime_error("'" + text + "' is not a supply: expected <degree>:<count>");
    }
    const std::string degree = text.substr(0, colon);
    const std::string count = text.substr(colon + 1);
    if (degree.empty() || count.empty() ||
        degree.find_first_not_of("0123456789") != std::string::npos ||
        count.find_first_not_of("0123456789") != std::string::npos) {
        throw std::runtime_error("'" + text + "' is not a supply: expected <degree>:<count>");
    }
    curve_bounds::PointSupply supply;
    supply.degree = std::stoull(degree);
    supply.available = std::stoull(count);
    if (supply.degree == 0) throw std::runtime_error("a point cannot have degree 0");
    return supply;
}

/// The transcribed table, as the paper prints it: `lower - upper`, a settled
/// entry with the two equal, and a dot where nothing is published. A zero lower
/// is the absence of a lower bound and is never printed as a zero.
void print_table() {
    cli::result() << "[rambaud2014, Table 1] as transcribed. Rows are l, columns m.\n"
                     "l = 1 is multiplication in GF(2^m) itself. '.' is unpublished,\n"
                     "and an entry shown as '- U' has an upper bound only.\n\n";
    cli::result() << "  l \\ m ";
    for (std::size_t degree = 1; degree <= 10; ++degree) {
        cli::result() << "     " << degree << (degree < 10 ? " " : "");
    }
    cli::result() << "\n";
    for (std::size_t truncation = 1; truncation <= 10; ++truncation) {
        cli::result() << "  " << (truncation < 10 ? " " : "") << truncation << "    ";
        for (std::size_t degree = 1; degree <= 10; ++degree) {
            const curve_bounds::Bound bound = curve_bounds::symmetric_bound(degree, truncation);
            std::string cell = ".";
            if (bound.known && bound.settled()) {
                cell = std::to_string(bound.upper);
            } else if (bound.known && bound.lower > 0) {
                cell = std::to_string(bound.lower) + "-" + std::to_string(bound.upper);
            } else if (bound.known) {
                cell = "-" + std::to_string(bound.upper);
            }
            cli::result() << "  " << cell
                          << std::string(cell.size() < 5 ? 5 - cell.size() : 0, ' ');
        }
        cli::result() << "\n";
    }
}

/// Which backends this machine has, best first, as `list-solvers` printed them.
///
/// It lives beside `--route` rather than in a command of its own because the
/// ranking is only ever read to decide what `--route chain` will reach: a caller
/// with no curve to bound has no use for it. The order printed is the one that
/// would really run: `run` puts `ilp_backend_order` in force before it walks a
/// single argument. Printing the compiled order while the file had moved it
/// would make the one place a caller looks the one place that lies.
void print_backends() {
    cli::result() << "integer programming backends, in preference order:\n";
    for (const integer_programme::Backend backend : integer_programme::ranked_backends()) {
        cli::result() << "  " << (integer_programme::is_available(backend) ? "present" : "absent ")
                      << "  " << integer_programme::name_of(backend) << "\n";
    }
    cli::result() << "\nThe first present backend answers; the built-in is always present and is"
                     " the\nonly one whose answer needs no checking, so it is also the only one"
                     " that may\nreport a problem infeasible.\n";
}

enum class Route { Chain, BuiltIn, Enumeration };

int run(int argc, char** argv) {
    long long divisor_degree = -1;
    std::vector<curve_bounds::PointSupply> supply;
    // The default is the route whose optimum is a proof and which was also the
    // fastest at every size measured. An outside solver's answer is only
    // feasibility-verified here, so it is asked for rather than fallen into.
    Route route = Route::BuiltIn;

    // The file first, so the two flags below overwrite what it said. The backend
    // order has no flag: --route is the coarser choice a caller makes here, and
    // the built-in is reached last whatever the order says.
    const cli::Tunables& tunables = cli::tunables();
    curve_bounds::set_solver_node_limit(tunables.ilp_node_limit);
    integer_programme::set_solver_time_limit(static_cast<unsigned>(tunables.ilp_time_limit_seconds));
    std::string unrecognised;
    if (!integer_programme::set_backend_order(tunables.ilp_backend_order, unrecognised)) {
        throw cli::ArgumentError("tunables.conf ilp_backend_order: no backend is called '" +
                                 unrecognised + "'");
    }

    // Walked by `infrastructure/cli/arguments.h` rather than by hand, so `--degree abc` names the
    // flag and the word rather than leaving as 5 saying `stoll`. `--points d:n`
    // keeps its own parser and its own exit code: an unreadable supply is not a
    // flag this command failed to recognise.
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--table")) {
            print_table();
            return cli::exit_status(cli::ExitCode::Yes);
        } else if (arguments.is("--solvers")) {
            print_backends();
            return cli::exit_status(cli::ExitCode::Yes);
        } else if (arguments.is("--degree")) {
            divisor_degree = arguments.whole_number();
        } else if (arguments.is("--points")) {
            supply.push_back(parse_supply(arguments.text()));
        } else if (arguments.is("--max-memory")) {
            // The frontier `plan_for` builds is quadratic in `--degree`, so the
            // one flag that could refuse it is this one, and the refusal
            // `require_room` writes names it by name.
            run_limits::set_memory_budget(arguments.memory_size());
        } else if (arguments.is("--node-limit")) {
            curve_bounds::set_solver_node_limit(arguments.count());
        } else if (arguments.is("--solver-timeout")) {
            integer_programme::set_solver_time_limit(static_cast<unsigned>(arguments.count()));
        } else if (arguments.is("--route")) {
            const std::string wanted = arguments.text();
            if (wanted == "chain") {
                route = Route::Chain;
            } else if (wanted == "built-in") {
                route = Route::BuiltIn;
            } else if (wanted == "enumeration") {
                route = Route::Enumeration;
            } else {
                usage();
                return cli::exit_status(cli::ExitCode::Usage);
            }
        } else {
            arguments.refuse();
        }
    }
    // The one command here with no input file: the whole input is the degree and
    // the supply. A word that is not a flag was silently dropped by the walk, and
    // `curve-bounds --degree 5 --points 1:8 stray` would then have printed a bound
    // as though the line had been understood.
    if (!arguments.filename().empty()) {
        throw cli::ArgumentError("nothing here reads a file, and '" + arguments.filename() +
                                 "' is not a flag");
    }

    if (divisor_degree < 0 || supply.empty()) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    cli::result() << "supply:";
    for (const curve_bounds::PointSupply& one : supply) {
        cli::result() << " " << one.available << " of degree " << one.degree;
    }
    cli::result() << "\ndivisor degree: " << divisor_degree << ", spent exactly\n";

    const std::size_t degree = static_cast<std::size_t>(divisor_degree);
    const curve_bounds::BoundResult programme =
        route == Route::Enumeration
            ? curve_bounds::minimise_interpolation_bound(supply, degree)
            : curve_bounds::minimise_interpolation_bound_by_solver(
                  supply, degree,
                  route == Route::BuiltIn ? curve_bounds::SolverChoice::BuiltInOnly
                                          : curve_bounds::SolverChoice::Chain);

    // Not solved is a real answer and not a failure: an effective divisor of that
    // degree cannot be assembled from that supply at prices the table publishes.
    if (!programme.solved) {
        cli::result() << "no divisor [" << programme.solved_by << "]: degree " << divisor_degree
                      << " cannot be made from this supply at any price the table publishes\n";
        return cli::exit_status(cli::ExitCode::No);
    }

    cli::result() << "bound [" << programme.solved_by << "]: mu_sym_2(m) <= " << programme.bound
                  << ", using";
    for (const curve_bounds::Selection& piece : programme.chosen) {
        cli::result() << " " << piece.count << "x(degree " << piece.degree << ", multiplicity "
                      << piece.multiplicity << ")";
    }
    cli::result() << "\n";
    // Two different weaknesses, and collapsing them would lose the one that can
    // be fixed by asking again. The envelope caveat is the method's and is
    // permanent here; an uncertified optimum is only this backend's, and
    // `--route built-in` settles it.
    if (!programme.optimum_proved) {
        cli::result() << "  feasible, not certified optimal: this backend's answer passed the"
                      << " model's own\n  checks, which cannot check optimality. --route built-in"
                      << " proves it.\n";
    }
    cli::result() << "  an envelope, not a bound: no curve with this supply was shown to exist\n";
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line, or a line of tunables.conf, that could not
        // be read: the run never started, so Usage rather than Error.
        cli::note() << "curve-bounds: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        cli::note() << "curve-bounds: " << problem.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
