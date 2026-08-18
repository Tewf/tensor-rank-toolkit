#include <iostream>
#include <string>

#include "exit_code.h"
#include "parallel.h"
#include "strict_deflation.h"
#include "symmetry_argument.h"
#include "tensor_file.h"

/// Pricing the refutation-based version of the same commitment.
///
/// [`find-at-rank`](find_at_rank_main.cpp) passes a candidate over when its budget
/// runs out. This one waits for a proof, which is what makes it
/// minimality-preserving and what makes it dear. The command exists to *measure*
/// that: `--refuter` picks which machine buys the refutation, and both are asked
/// about the same candidates in the same order so that the difference is the
/// machine and not the question.
namespace {

void usage() {
    std::cerr << "usage: deflate-strictly <tensor-file> --target k -s matmul <n> <m> <k>\n"
                 "\n"
                 "  --target k          the rank to test the candidates against\n"
                 "  --refuter solver|tree\n"
                 "                      solver waits for unsatisfiable on each cube;\n"
                 "                      tree walks expand_subspace_up_to on the span\n"
                 "                      enlarged by the candidate. Default solver\n"
                 "  --candidate-timeout N\n"
                 "                      seconds per candidate on the solver route, 300\n"
                 "                      by default. The tree route is bounded by nodes\n"
                 "  --node-limit N      nodes the tree may visit, 5000000 by default\n"
                 "  --parallel          ask the candidates on all cores. Prices every\n"
                 "                      candidate rather than stopping at the first yes\n"
                 "  --break-symmetry    order terms 1 onward, which is what a matched\n"
                 "                      baseline against the whole instance needs\n"
                 "  --solver <name>     pin a SAT solver\n"
              << cli::symmetry_usage()
              << "\n"
                 "  exit: 0 accepted  1 refuted  2 usage  3 undecided  4 unverified  5 error\n";
}

int run(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return cli::as_int(cli::ExitCode::Usage);
    }
    const std::string path = argv[1];
    if (path.rfind("--", 0) == 0 || path == "-h") {
        usage();
        return cli::as_int(cli::ExitCode::Usage);
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(path);
    bilinear_rank::StrictSettings settings;
    cli::Symmetry symmetry;
    long long target = -1;

    for (int argument = 2; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (option == "--target" && argument + 1 < argc) {
            target = std::stoll(argv[++argument]);
        } else if (option == "--refuter" && argument + 1 < argc) {
            const std::string named = argv[++argument];
            if (named == "tree") {
                settings.refuter = bilinear_rank::Refuter::QuotientedTree;
            } else if (named != "solver") {
                usage();
                return cli::as_int(cli::ExitCode::Usage);
            }
        } else if (option == "--candidate-timeout" && argument + 1 < argc) {
            settings.candidate_seconds = std::stoull(argv[++argument]);
        } else if (option == "--node-limit" && argument + 1 < argc) {
            settings.node_limit = std::stoull(argv[++argument]);
        } else if (option == "--parallel") {
            settings.parallel_candidates = true;
            bilinear_rank::set_worker_count(0);
        } else if (option == "--break-symmetry") {
            settings.approach.break_symmetry = true;
        } else if (option == "--solver" && argument + 1 < argc) {
            settings.approach.solver = argv[++argument];
        } else if (option == "--symmetry" || option == "-s") {
            symmetry = cli::parse_symmetry(argc, argv, argument);
        } else {
            usage();
            return cli::as_int(cli::ExitCode::Usage);
        }
    }
    if (target < 1) {
        usage();
        return cli::as_int(cli::ExitCode::Usage);
    }
    settings.matmul_shape = symmetry.shape;

    const bilinear_rank::StrictStep step =
        bilinear_rank::strict_step(tensor, static_cast<std::size_t>(target), settings);

    const char* name[] = {"accepted", "refuted", "undecided"};
    for (const bilinear_rank::CandidateVerdict& verdict : step.verdicts) {
        std::cout << "  candidate " << verdict.candidate << ": "
                  << name[static_cast<int>(verdict.verdict)] << ", " << verdict.seconds << " s";
        if (verdict.nodes > 0) std::cout << ", " << verdict.nodes << " nodes";
        std::cout << "\n";
    }
    std::cout << "  k = " << step.products << ": ";
    if (step.accepted) {
        std::cout << "accepted, " << step.decomposition.size() << " products";
    } else if (step.refuted) {
        std::cout << "every candidate refuted, so the rank is above " << step.products;
    } else {
        std::cout << "undecided: at least one candidate ran out of budget, and that is "
                     "not a no";
    }
    std::cout << ", " << step.seconds << " s";
    std::cout << (step.solver_name.empty() ? ", quotiented tree" : ", " + step.solver_name);
    std::cout << "\n";

    if (step.accepted) return cli::as_int(cli::ExitCode::Yes);
    if (step.refuted) return cli::as_int(cli::ExitCode::No);
    return cli::as_int(cli::ExitCode::Undecided);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::CheckFailed& failure) {
        std::cerr << "deflate-strictly: " << failure.what() << "\n";
        return cli::as_int(cli::ExitCode::Unverified);
    } catch (const std::exception& error) {
        std::cerr << "deflate-strictly: " << error.what() << "\n";
        return cli::as_int(cli::ExitCode::Error);
    }
}
