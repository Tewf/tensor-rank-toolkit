/// `factor-over-canonical-basis`: the rank as `S = C A`, with both factors.
///
/// The other rank commands print a number and the products. This one prints the
/// factorisation the number *is*: `A` over the canonical basis, `C` beside it,
/// and the statement that their product is the tensor. A reader with a matrix
/// library and no trust in this repository can check the answer in one line.
#include <exception>
#include <iostream>
#include <string>

#include "arguments.h"
#include "symmetry_argument.h"
#include "exit_code.h"
#include "factorisation.h"
#include "tensor_file.h"
#include "timing.h"
#include "tunables.h"

namespace {

void write_matrix(const char* label, const linear_algebra::ModularMatrix& matrix) {
    std::cout << label << ": " << matrix.rows() << "x" << matrix.columns() << "\n";
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        std::cout << " ";
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            std::cout << " " << matrix(row, column);
        }
        std::cout << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: factor-over-canonical-basis <tensor-file> [--floor k]\n"
                  << "                                   [--node-limit n] [--symmetry none]\n"
                  << "                                   [--route auto|exhaustive|sat|canonical]\n"
                  << cli::symmetry_usage()
                  << "  Writes A over the canonical basis, every row a rank-one matrix, and\n"
                  << "  the C with C A equal to the tensor's slices. The number of rows of A\n"
                  << "  is the rank when the sweep below it was complete.\n";
        return cli::exit_status(cli::ExitCode::Usage);
    }

    // A flag where the tensor should be is a bad invocation, not an unreadable
    // file. Without this, `--route bogus` reports that it cannot read a file
    // called `--route`, which names the wrong thing to fix and exits 5 where a
    // script watching for 2 would not see it.
    if (cli::looks_like_flag(argv[1])) {
        std::cerr << "factor-over-canonical-basis: expected a tensor file, not '" << argv[1] << "'\n";
        return cli::exit_status(cli::ExitCode::Usage);
    }

    try {
        const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(argv[1]);
        const linear_algebra::ModularField field(tensor.characteristic);

        canonical_factorisation::FactorisationSettings settings;
        settings.node_limit = cli::tunables().search_node_limit;
        for (int argument = 2; argument < argc; ++argument) {
            const std::string option = argv[argument];
            if (option == "--floor" && argument + 1 < argc) {
                settings.floor = cli::parse_count(option, argv[++argument]);
            } else if (option == "--node-limit" && argument + 1 < argc) {
                settings.node_limit = cli::parse_count(option, argv[++argument]);
            } else if (option == "--route" && argument + 1 < argc) {
                const std::string named = argv[++argument];
                if (named == "auto") {
                    settings.route = canonical_factorisation::Route::Automatic;
                } else if (named == "exhaustive") {
                    settings.route = canonical_factorisation::Route::Exhaustive;
                } else if (named == "sat") {
                    settings.route = canonical_factorisation::Route::Satisfiability;
                } else if (named == "canonical") {
                    settings.route = canonical_factorisation::Route::CanonicalAugmentation;
                } else {
                    std::cerr << "factor-over-canonical-basis: --route takes auto, exhaustive, "
                                 "sat or canonical, not '" << named << "'\n";
                    return cli::exit_status(cli::ExitCode::Usage);
                }
            } else if (option == "--symmetry" || option == "-s") {
                settings.symmetry = cli::parse_symmetry(argc, argv, argument);
            } else {
                std::cerr << "factor-over-canonical-basis: unrecognised option '" << option
                          << "'\n";
                return cli::exit_status(cli::ExitCode::Usage);
            }
        }

        const cli::Clock::time_point started = cli::Clock::now();
        const canonical_factorisation::Factorisation factorisation =
            canonical_factorisation::factor_over_canonical_basis(field, tensor.slices, settings);

        std::cerr << "# " << cli::elapsed_seconds(started) << " s\n";

        if (factorisation.components == 0 && !tensor.slices.empty()) {
            std::cerr << "factor-over-canonical-basis: no factorisation found up to the ceiling\n";
            return cli::exit_status(cli::ExitCode::Undecided);
        }

        // Checked before it is printed. An answer this command has not verified
        // is worth less than no answer, because it looks exactly the same.
        if (!canonical_factorisation::recovers_slices(field, tensor.slices, factorisation)) {
            std::cerr << "factor-over-canonical-basis: C A does not give the slices back\n";
            return cli::exit_status(cli::ExitCode::Unverified);
        }

        std::cout << "tensor: " << argv[1] << ", GF(" << tensor.characteristic << "), "
                  << tensor.slices.size() << " slices of " << tensor.rows() << "x"
                  << tensor.columns() << "\n";
        std::cout << "floor: " << factorisation.floor << " (proved)\n";
        if (!factorisation.symmetry_refusal.empty()) {
            std::cerr << "# no symmetry: " << factorisation.symmetry_refusal << "\n";
        }
        const char* route_name = "exhaustion over the materialised pool";
        if (factorisation.route == canonical_factorisation::Route::Satisfiability) {
            route_name = "a SAT solver, which formed no pool";
        } else if (factorisation.route == canonical_factorisation::Route::CanonicalAugmentation) {
            route_name = "exhaustion with canonical augmentation";
        }
        std::cout << "route: " << route_name
                  << ", where the pool would be " << factorisation.pool_size << " matrices\n";
        if (factorisation.route != canonical_factorisation::Route::Satisfiability) {
            std::cout << "symmetry: quotiented by " << factorisation.group_size
                      << " automorphisms, " << factorisation.nodes_visited
                      << " nodes over the sweep\n";
        }
        write_matrix("A, over the canonical basis", factorisation.chosen);
        write_matrix("C, the recovery", factorisation.recovery);
        std::cout << "checked: every row of A has rank 1, and C A is the tensor\n";
        std::cout << "components: " << factorisation.components
                  << (factorisation.minimal ? " (the rank)" : " (an upper bound: a budget ran out)")
                  << "\n";

        return cli::exit_status(factorisation.minimal ? cli::ExitCode::Yes
                                                      : cli::ExitCode::Undecided);
    } catch (const cli::ArgumentError& error) {
        std::cerr << "factor-over-canonical-basis: " << error.what() << "\n";
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const cli::CheckFailed& error) {
        std::cerr << "factor-over-canonical-basis: " << error.what() << "\n";
        return cli::exit_status(cli::ExitCode::Unverified);
    } catch (const std::exception& error) {
        std::cerr << "factor-over-canonical-basis: " << error.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }
}
