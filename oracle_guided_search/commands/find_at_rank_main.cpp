#include <iostream>
#include <string>

#include "descending_sweep.h"
#include "exit_code.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "tensor_flattening.h"

/// Producing an upper bound without ever buying a refutation.
///
/// `decide-rank-by-sat` sweeps `k` upward and pays for a refusal at every rank
/// below the answer. This sweeps downward and pays for none, because it never
/// waits for one: see [`fixed_rank_finder.h`](../fixed_rank_finder.h) for the
/// completeness that costs.
///
/// The exit codes are the shared ones in [`exit_code.h`](../../cli/exit_code.h),
/// and `Undecided` is the ordinary outcome of a run that found nothing rather than
/// an error: nothing was proved, which is not the same as a no.
namespace {

void usage() {
    std::cerr << "usage: find-at-rank <tensor-file> --target k\n"
                 "       find-at-rank <tensor-file> --descend\n"
                 "\n"
                 "  --target k          ask for a k-product algorithm and stop\n"
                 "  --descend           walk k down from the naive ceiling, reporting\n"
                 "                      the least k reached and the bracket to hand\n"
                 "                      decide-rank-by-sat for the one refutation left\n"
                 "  --ceiling N         start the descent below the naive cost\n"
                 "  --candidate-timeout N\n"
                 "                      seconds per candidate, 30 by default. A candidate\n"
                 "                      that exhausts it is passed over, never refuted\n"
                 "  --floor N           a lower bound proved elsewhere; the flattening\n"
                 "                      bound is used when this is not given\n"
                 "  --solver <name>     pin a SAT solver instead of taking the best fit\n"
                 "  --break-symmetry    order terms 1 onward; sound beside a cube, which\n"
                 "                      pins term 0 and orders nothing\n"
              << cli::symmetry_usage()
              << "\n"
                 "  exit: 0 found  2 usage  3 nothing found  4 unverified  5 error\n";
}

/// One line per candidate, so a sweep can be priced rather than merely timed.
void report_candidates(const bilinear_rank::FoundAtRank& step) {
    const char* name[] = {"yes", "no", "unknown"};
    for (std::size_t index = 0; index < step.per_candidate.verdict.size(); ++index) {
        std::cout << "    candidate " << index << ": "
                  << name[static_cast<int>(step.per_candidate.verdict[index])] << ", "
                  << step.per_candidate.seconds[index] << " s\n";
    }
}

void report_step(const bilinear_rank::FoundAtRank& step) {
    std::cout << "  k = " << step.products << ": ";
    switch (step.outcome) {
        case bilinear_rank::Outcome::Found:
            std::cout << "found, " << step.decomposition.size() << " products, candidate "
                      << step.accepted_candidate << " of " << step.candidates;
            break;
        case bilinear_rank::Outcome::FoundWithoutSolver:
            std::cout << "found by the polynomial pre-test, " << step.decomposition.size()
                      << " products, no solver run";
            break;
        case bilinear_rank::Outcome::BelowFloor:
            std::cout << "below the floor, so no algorithm of this size exists";
            break;
        case bilinear_rank::Outcome::NotFound:
            std::cout << "not found after " << step.candidates_asked << " of " << step.candidates
                      << " candidates";
            if (step.oracle_verdict == satisfiability::Verdict::No) {
                std::cout << ", every one refused (a refutation, unproved)";
            } else {
                std::cout << " (nothing proved: this is not a no)";
            }
            break;
    }
    std::cout << ", " << step.seconds << " s";
    if (!step.solver_name.empty()) std::cout << ", " << step.solver_name;
    std::cout << "\n";
    report_candidates(step);
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
    bilinear_rank::FinderSettings settings;
    cli::Symmetry symmetry;
    long long target = -1;
    long long ceiling = -1;
    bool descend = false;
    bool floor_given = false;

    for (int argument = 2; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (option == "--target" && argument + 1 < argc) {
            target = std::stoll(argv[++argument]);
        } else if (option == "--ceiling" && argument + 1 < argc) {
            ceiling = std::stoll(argv[++argument]);
        } else if (option == "--descend") {
            descend = true;
        } else if (option == "--candidate-timeout" && argument + 1 < argc) {
            settings.candidate_seconds = std::stoull(argv[++argument]);
        } else if (option == "--floor" && argument + 1 < argc) {
            settings.floor = std::stoull(argv[++argument]);
            floor_given = true;
        } else if (option == "--solver" && argument + 1 < argc) {
            settings.approach.solver = argv[++argument];
        } else if (option == "--break-symmetry") {
            settings.approach.break_symmetry = true;
        } else if (option == "--symmetry" || option == "-s") {
            symmetry = cli::parse_symmetry(argc, argv, argument);
        } else {
            usage();
            return cli::as_int(cli::ExitCode::Usage);
        }
    }

    if (symmetry.kind == cli::SymmetryKind::Automatic) {
        std::cerr << "find-at-rank: --symmetry auto has no meaning here. The candidates are the\n"
                     "closed-form orbits of a matrix multiplication tensor, so name the shape:\n"
                     "--symmetry matmul <n> <m> <k>\n";
        return cli::as_int(cli::ExitCode::Usage);
    }
    settings.matmul_shape = symmetry.shape;

    const bilinear_rank::Field field(tensor.characteristic);
    if (!floor_given) {
        settings.floor = linear_algebra::flattening_lower_bound(field, tensor.slices);
    }
    std::cout << "  floor: " << settings.floor << (floor_given ? " (given)" : " (flattening)")
              << "\n";
    if (!settings.matmul_shape.empty() && tensor.characteristic != 2) {
        std::cout << "  no commitment: a cube is GF(2) only, so this asks one unrestricted\n"
                     "  question per k\n";
    }

    if (descend) {
        const std::size_t from = ceiling > 0 ? static_cast<std::size_t>(ceiling)
                                             : bilinear_rank::naive_ceiling(tensor);
        std::cout << "  descending from " << from << "\n";
        const bilinear_rank::SweepResult sweep =
            bilinear_rank::descend_from_ceiling(tensor, from, settings);
        for (const bilinear_rank::FoundAtRank& step : sweep.steps) report_step(step);
        if (sweep.upper == 0) {
            std::cout << "  nothing found, " << sweep.seconds << " s\n";
            return cli::as_int(cli::ExitCode::Undecided);
        }
        std::cout << "  upper bound " << sweep.upper << ", bracket [" << sweep.floor << ", "
                  << sweep.upper << "], " << sweep.seconds << " s\n";
        return cli::as_int(cli::ExitCode::Yes);
    }

    if (target < 1) {
        usage();
        return cli::as_int(cli::ExitCode::Usage);
    }
    const bilinear_rank::FoundAtRank step =
        bilinear_rank::find_at_rank(tensor, static_cast<std::size_t>(target), settings);
    report_step(step);
    if (bilinear_rank::was_found(step.outcome)) return cli::as_int(cli::ExitCode::Yes);
    if (step.outcome == bilinear_rank::Outcome::BelowFloor) {
        return cli::as_int(cli::ExitCode::No);
    }
    return cli::as_int(cli::ExitCode::Undecided);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::CheckFailed& failure) {
        std::cerr << "find-at-rank: " << failure.what() << "\n";
        return cli::as_int(cli::ExitCode::Unverified);
    } catch (const std::exception& error) {
        std::cerr << "find-at-rank: " << error.what() << "\n";
        return cli::as_int(cli::ExitCode::Error);
    }
}
