#include <string>

#include "arguments.h"
#include "exit_code.h"
#include "parallel.h"
#include "report.h"
#include "strict_deflation.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "tunables.h"

/// Pricing the refutation-based version of the same commitment.
///
/// The fixed-`k` finder passed a candidate over when its budget
/// runs out. This one waits for a proof, which is what makes it
/// minimality-preserving and what makes it dear. The command exists to *measure*
/// that: `--refuter` picks which machine buys the refutation, and both are asked
/// about the same candidates in the same order so that the difference is the
/// machine and not the question.
namespace {

void usage() {
    cli::note() << "usage: deflate-strictly <tensor-file> --target k -s matmul <n> <m> <k>\n"
                   "\n"
                   "  --target k          the rank to test the candidates against\n"
                   "  --refuter solver|tree\n"
                   "                      solver waits for unsatisfiable on each cube;\n"
                   "                      tree walks expand_subspace_up_to_symmetry on the span\n"
                   "                      enlarged by the candidate. Default solver\n"
                   "  --candidate-timeout N\n"
                   "                      seconds per candidate on the solver route, from\n"
                   "                      sat_timeout_seconds in tunables.conf when this is\n"
                   "                      not given. The tree route is bounded by nodes\n"
                   "  --node-limit N      nodes the tree may visit, from search_node_limit\n"
                   "                      in tunables.conf when this is not given\n"
                   "  --max-memory 2G     cap on the solver, from sat_memory_megabytes in\n"
                   "                      tunables.conf when this is not given\n"
                   "  --parallel          ask the candidates on all cores. Prices every\n"
                   "                      candidate rather than stopping at the first yes\n"
                   "  --break-symmetry    order terms 1 onward, which is what a matched\n"
                   "                      baseline against the whole instance needs\n"
                   "  --solver <name>     pin a SAT solver\n"
                   "  --help              print this and stop, as exit 2\n"
                << cli::symmetry_usage()
                << "\n"
                   "  exit: 0 accepted  1 refuted  2 usage  3 undecided  4 unverified  5 error";
}

int run(int argc, char** argv) {
    bilinear_rank::StrictSettings settings;
    // The file first, so every flag below overwrites what it said. A refutation
    // needs the large budget, so the candidate clock takes the SAT timeout
    // rather than a smaller one of its own.
    const cli::Tunables& tunables = cli::tunables();
    settings.candidate_seconds = tunables.sat_timeout_seconds;
    settings.node_limit = tunables.search_node_limit;
    settings.approach.memory_megabytes = tunables.sat_memory_megabytes;
    settings.approach.solver_order = tunables.sat_solver_order;
    cli::Symmetry symmetry;
    long long target = -1;

    // Walked by `cli/arguments.h` rather than by hand, so `--target abc` names the
    // flag and the word rather than leaving as 5 saying `stoll`, and the file is
    // read after the line is understood rather than before: `--help` cannot open
    // a tensor, and a flag where the path belongs is not a missing file.
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--target")) {
            target = arguments.whole_number();
        } else if (arguments.is("--refuter")) {
            const std::string named = arguments.text();
            if (named == "tree") {
                settings.refuter = bilinear_rank::Refuter::QuotientedTree;
            } else if (named != "solver") {
                usage();
                return cli::exit_status(cli::ExitCode::Usage);
            }
        } else if (arguments.is("--candidate-timeout")) {
            settings.candidate_seconds = arguments.count();
        } else if (arguments.is("--node-limit")) {
            settings.node_limit = arguments.count();
        } else if (arguments.is("--max-memory")) {
            settings.approach.memory_megabytes = arguments.memory_size() / (1024 * 1024);
        } else if (arguments.is("--parallel")) {
            settings.parallel_candidates = true;
            bilinear_rank::set_worker_count(0);
        } else if (arguments.is("--break-symmetry")) {
            settings.approach.break_symmetry = true;
        } else if (arguments.is("--solver")) {
            settings.approach.solver = arguments.text();
        } else if (arguments.is("--symmetry", "-s")) {
            symmetry = arguments.parsed_by(cli::parse_symmetry);
        } else {
            arguments.refuse();
        }
    }
    // No file named, and nothing here reads a tensor on stdin.
    if (arguments.reads_stdin() || target < 1) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    settings.matmul_shape = symmetry.shape;

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(arguments.filename());

    const bilinear_rank::StrictStep step =
        bilinear_rank::strict_step(tensor, static_cast<std::size_t>(target), settings);

    const char* name[] = {"accepted", "refuted", "undecided"};
    for (const bilinear_rank::CandidateVerdict& verdict : step.verdicts) {
        cli::result() << "  candidate " << verdict.candidate << ": "
                      << name[static_cast<int>(verdict.verdict)] << ", " << verdict.seconds << " s";
        if (verdict.nodes > 0) cli::result() << ", " << verdict.nodes << " nodes";
        cli::result() << "\n";
    }
    cli::result() << "  k = " << step.products << ": ";
    if (step.accepted) {
        cli::result() << "accepted, " << step.decomposition.size() << " products";
    } else if (step.refuted) {
        cli::result() << "every candidate refuted, so the rank is above " << step.products;
    } else {
        cli::result() << "undecided: at least one candidate ran out of budget, and that is "
                         "not a no";
    }
    cli::result() << ", " << step.seconds << " s";
    cli::result() << (step.solver_name.empty() ? ", quotiented tree" : ", " + step.solver_name);
    cli::result() << "\n";

    if (step.accepted) return cli::exit_status(cli::ExitCode::Yes);
    if (step.refuted) return cli::exit_status(cli::ExitCode::No);
    return cli::exit_status(cli::ExitCode::Undecided);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::CheckFailed& failure) {
        cli::note() << "deflate-strictly: " << failure.what();
        return cli::exit_status(cli::ExitCode::Unverified);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line, or a line of tunables.conf, that could not
        // be read: the run never started, so Usage rather than Error.
        cli::note() << "deflate-strictly: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& error) {
        cli::note() << "deflate-strictly: " << error.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
