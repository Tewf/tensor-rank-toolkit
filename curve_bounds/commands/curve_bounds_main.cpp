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
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "interpolation_by_solver.h"
#include "interpolation_programme.h"
#include "symmetric_bound_table.h"

namespace {

void usage() {
    std::cerr << "usage: curve-bounds --degree G --points d:n [--points d:n ...]\n"
                 "       curve-bounds --table\n"
                 "\n"
                 "  --degree G      the divisor's degree, spent exactly and not as a budget.\n"
                 "                  Every point costs something, so minimising over smaller\n"
                 "                  divisors too would always answer 'one rational point,\n"
                 "                  cost 1', which is a bound on nothing.\n"
                 "  --points d:n    n distinct closed points of degree d are available. Repeat\n"
                 "                  for each degree. This is step 2's output, and step 2 is\n"
                 "                  not in this repository.\n"
                 "  --table         print [rambaud2014, Table 1] as transcribed, and stop\n"
                 "  --route built-in|chain|enumeration\n"
                 "                  which minimiser answers. built-in is the default: exact\n"
                 "                  branch and bound in rationals, whose optimum is a proof,\n"
                 "                  and the fastest of the three at every size measured.\n"
                 "                  chain asks the first installed MILP backend, whose point\n"
                 "                  the model accepts but whose optimality it cannot check.\n"
                 "                  enumeration is the dynamic programme, exact and the\n"
                 "                  cross-check.\n"
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
    std::cout << "[rambaud2014, Table 1] as transcribed. Rows are l, columns m.\n"
                 "l = 1 is multiplication in GF(2^m) itself. '.' is unpublished,\n"
                 "and an entry shown as '- U' has an upper bound only.\n\n";
    std::cout << "  l \\ m ";
    for (std::size_t degree = 1; degree <= 10; ++degree) {
        std::cout << "     " << degree << (degree < 10 ? " " : "");
    }
    std::cout << "\n";
    for (std::size_t truncation = 1; truncation <= 10; ++truncation) {
        std::cout << "  " << (truncation < 10 ? " " : "") << truncation << "    ";
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
            std::cout << "  " << cell << std::string(cell.size() < 5 ? 5 - cell.size() : 0, ' ');
        }
        std::cout << "\n";
    }
}

enum class Route { Chain, BuiltIn, Enumeration };

int run(int argc, char** argv) {
    long long divisor_degree = -1;
    std::vector<curve_bounds::PointSupply> supply;
    // The default is the route whose optimum is a proof and which was also the
    // fastest at every size measured. An outside solver's answer is only
    // feasibility-verified here, so it is asked for rather than fallen into.
    Route route = Route::BuiltIn;

    for (int argument = 1; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (option == "--table") {
            print_table();
            return 0;
        }
        if (option == "--degree" && argument + 1 < argc) {
            divisor_degree = std::stoll(argv[++argument]);
        } else if (option == "--points" && argument + 1 < argc) {
            supply.push_back(parse_supply(argv[++argument]));
        } else if (option == "--route" && argument + 1 < argc) {
            const std::string wanted = argv[++argument];
            if (wanted == "chain") {
                route = Route::Chain;
            } else if (wanted == "built-in") {
                route = Route::BuiltIn;
            } else if (wanted == "enumeration") {
                route = Route::Enumeration;
            } else {
                usage();
                return 2;
            }
        } else {
            usage();
            return 2;
        }
    }

    if (divisor_degree < 0 || supply.empty()) {
        usage();
        return 2;
    }

    std::cout << "supply:";
    for (const curve_bounds::PointSupply& one : supply) {
        std::cout << " " << one.available << " of degree " << one.degree;
    }
    std::cout << "\ndivisor degree: " << divisor_degree << ", spent exactly\n";

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
        std::cout << "no divisor [" << programme.solved_by << "]: degree " << divisor_degree
                  << " cannot be made from this supply at any price the table publishes\n";
        return 1;
    }

    std::cout << "bound [" << programme.solved_by << "]: mu_sym_2(m) <= " << programme.bound
              << ", using";
    for (const curve_bounds::Selection& piece : programme.chosen) {
        std::cout << " " << piece.count << "x(degree " << piece.degree << ", multiplicity "
                  << piece.multiplicity << ")";
    }
    std::cout << "\n";
    // Two different weaknesses, and collapsing them would lose the one that can
    // be fixed by asking again. The envelope caveat is the method's and is
    // permanent here; an uncertified optimum is only this backend's, and
    // `--route built-in` settles it.
    if (!programme.optimum_proved) {
        std::cout << "  feasible, not certified optimal: this backend's answer passed the model's"
                  << " own\n  checks, which cannot check optimality. --route built-in proves it.\n";
    }
    std::cout << "  an envelope, not a bound: no curve with this supply was shown to exist\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& problem) {
        std::cerr << "curve-bounds: " << problem.what() << "\n";
        return 5;
    }
}
