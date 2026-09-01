/// `factor-over-canonical-basis`: the rank as `S = C A`, with both factors.
///
/// The other rank commands print a number and the products. This one prints the
/// factorisation the number *is*: `A` over the canonical basis, `C` beside it,
/// and the statement that their product is the tensor. A reader with a matrix
/// library and no trust in this repository can check the answer in one line.
#include <exception>
#include <string>

#include "arguments.h"
#include "symmetry_argument.h"
#include "exit_code.h"
#include "memory_budget.h"
#include "parallel.h"
#include "factorisation.h"
#include "report.h"
#include "tensor_file.h"
#include "timing.h"
#include "tunables.h"

namespace {

void usage() {
    cli::note() << "usage: factor-over-canonical-basis <tensor-file> [--floor k] [--ceiling k]\n"
                   "                                   [--node-limit n] [--symmetry none]\n"
                   "                                   [--route auto|exhaustive|sat|canonical]\n"
                   "                                   [--threads N] [--max-memory N] [--help]\n"
                << cli::symmetry_usage()
                << "  Writes A over the canonical basis, every row a rank-one matrix, and\n"
                   "  the C with C A equal to the tensor's slices. The number of rows of A\n"
                   "  is the rank when the sweep below it was complete.\n"
                   "  --ceiling k  stop the sweep at k instead of at the sum of the slices'\n"
                   "               ranks, so a run costs a stated number of levels. Below the\n"
                   "               rank it refutes and reports what that cost\n"
                   "  --help  print this and stop, as exit 2";
}

/// What the sweep was and what it cost, printed whether or not it found anything.
///
/// A refuted sweep is a measurement and not a non-event: its node count is the
/// whole of what a route charged for the levels it walked out, and comparing two
/// routes on the levels below the rank is the only comparison that reaches a shape
/// whose rank neither of them can settle. This used to print after the
/// found-a-factorisation gate, so a refutation reported one sentence and no number.
void write_sweep(const std::string& path, const formats::Tensor& tensor,
                 const canonical_factorisation::Factorisation& factorisation) {
    cli::result() << "tensor: " << path << ", GF(" << tensor.characteristic << "), "
                  << tensor.slices.size() << " slices of " << tensor.rows() << "x"
                  << tensor.columns() << "\n";
    cli::result() << "floor: " << factorisation.floor << " (proved)\n";
    if (!factorisation.symmetry_refusal.empty()) {
        cli::note() << "no symmetry: " << factorisation.symmetry_refusal;
    }
    const char* route_name = "exhaustion over the materialised pool";
    if (factorisation.route == canonical_factorisation::Route::Satisfiability) {
        route_name = "a SAT solver, which formed no pool";
    } else if (factorisation.route == canonical_factorisation::Route::CanonicalAugmentation) {
        route_name = "exhaustion with canonical augmentation";
    }
    cli::result() << "route: " << route_name << ", where the pool would be "
                  << factorisation.pool_size << " matrices\n";
    if (factorisation.route != canonical_factorisation::Route::Satisfiability) {
        cli::result() << "symmetry: quotiented by " << factorisation.group_size
                      << " automorphisms, " << factorisation.nodes_visited
                      << " nodes over the sweep\n";
    }
    if (factorisation.route == canonical_factorisation::Route::CanonicalAugmentation) {
        cli::result() << "parent test: " << factorisation.canonisations
                      << " canonical images over the sweep\n";
    }
}

void write_matrix(const char* label, const linear_algebra::ModularMatrix& matrix) {
    cli::result() << label << ": " << matrix.rows() << "x" << matrix.columns() << "\n";
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        cli::result() << " ";
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            cli::result() << " " << matrix(row, column);
        }
        cli::result() << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        canonical_factorisation::FactorisationSettings settings;
        settings.node_limit = cli::tunables().search_node_limit;
        // Walked by `cli/arguments.h` rather than by hand. The walker is also
        // where the hand-written test for a flag where the tensor belongs went,
        // and the file is read after the line is understood rather than before:
        // `--help` cannot open a tensor, and `--route bogus` names the flag to fix
        // rather than a file called `--route` that nobody meant to open.
        cli::Arguments arguments(argc, argv);
        while (arguments.next_flag()) {
            if (arguments.is("--help", "-h")) {
                usage();
                return cli::exit_status(cli::ExitCode::Usage);
            } else if (arguments.is("--floor")) {
                settings.floor = arguments.count();
            } else if (arguments.is("--ceiling")) {
                settings.ceiling = arguments.count();
            } else if (arguments.is("--node-limit")) {
                settings.node_limit = arguments.count();
            } else if (arguments.is("--threads")) {
                // Only the pool routes have anything to spread: `expand_subspace`
                // runs one subtree per pool element. The solver route is one
                // process and the canonical route recurses through a shared
                // parent test, so neither reads this and both say so rather than
                // appearing to accept it.
                run_limits::set_worker_count(arguments.count());
            } else if (arguments.is("--max-memory")) {
                run_limits::set_memory_budget(arguments.memory_size());
            } else if (arguments.is("--route")) {
                const std::string named = arguments.text();
                if (named == "auto") {
                    settings.route = canonical_factorisation::Route::Automatic;
                } else if (named == "exhaustive") {
                    settings.route = canonical_factorisation::Route::Exhaustive;
                } else if (named == "sat") {
                    settings.route = canonical_factorisation::Route::Satisfiability;
                } else if (named == "canonical") {
                    settings.route = canonical_factorisation::Route::CanonicalAugmentation;
                } else {
                    cli::note() << "factor-over-canonical-basis: --route takes auto, exhaustive, "
                                   "sat or canonical, not '" << named << "'";
                    return cli::exit_status(cli::ExitCode::Usage);
                }
            } else if (arguments.is("--symmetry", "-s")) {
                settings.symmetry = arguments.parsed_by(cli::parse_symmetry);
            } else {
                arguments.refuse();
            }
        }
        // No file named, and nothing here reads a tensor on stdin.
        if (arguments.reads_stdin()) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
        const std::string path = arguments.filename();

        const formats::Tensor tensor = formats::read_tensor_file(path);
        const linear_algebra::ModularField field(tensor.characteristic);

        const cli::Clock::time_point started = cli::Clock::now();
        const canonical_factorisation::Factorisation factorisation =
            canonical_factorisation::factor_over_canonical_basis(field, tensor.slices, settings);

        cli::note() << cli::elapsed_seconds(started) << " s";

        if (factorisation.components == 0 && !tensor.slices.empty()) {
            write_sweep(path, tensor, factorisation);
            cli::note() << "factor-over-canonical-basis: no factorisation found up to the "
                           "ceiling";
            return cli::exit_status(cli::ExitCode::Undecided);
        }

        // Checked before it is printed. An answer this command has not verified
        // is worth less than no answer, because it looks exactly the same.
        if (!canonical_factorisation::recovers_slices(field, tensor.slices, factorisation)) {
            cli::note() << "factor-over-canonical-basis: C A does not give the slices back";
            return cli::exit_status(cli::ExitCode::Unverified);
        }

        write_sweep(path, tensor, factorisation);
        write_matrix("A, over the canonical basis", factorisation.chosen);
        write_matrix("C, the recovery", factorisation.recovery);
        cli::result() << "checked: every row of A has rank 1, and C A is the tensor\n";
        cli::result() << "components: " << factorisation.components
                      << (factorisation.minimal ? " (the rank)"
                                                : " (an upper bound: a budget ran out)")
                      << "\n";

        return cli::exit_status(factorisation.minimal ? cli::ExitCode::Yes
                                                      : cli::ExitCode::Undecided);
    } catch (const cli::ArgumentError& error) {
        cli::note() << "factor-over-canonical-basis: " << error.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const cli::CheckFailed& error) {
        cli::note() << "factor-over-canonical-basis: " << error.what();
        return cli::exit_status(cli::ExitCode::Unverified);
    } catch (const std::exception& error) {
        cli::note() << "factor-over-canonical-basis: " << error.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
