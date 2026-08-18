/// Run the heuristic on a bilinear map and report what each step cost.
///
/// Takes a file and arguments so runs can be scripted, timed, and repeated,
/// rather than interactive.
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "arguments.h"
#include "candidate_pool.h"
#include "exit_code.h"
#include "fewest_products.h"
#include "group_construction.h"
#include "sms_file.h"
#include "memory_budget.h"
#include "minimise_rank.h"
#include "orbit_heuristic.h"
#include "parallel.h"
#include "plateau_search.h"
#include "size_argument.h"
#include "requested_group.h"
#include "minimum_weight_basis.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "timing.h"
#include "tunables.h"

namespace {

/// Checked after every step, not only in the tests: the result has to still
/// generate the map it came from, or the number means nothing.
bool verify(const bilinear_rank::Field& field, const std::vector<bilinear_rank::Matrix>& current,
            const std::vector<bilinear_rank::Matrix>& original, const std::string& step) {
    if (linear_algebra::spans_all(field, current, original)) return true;
    std::cerr << "FAILED: after " << step << " the result no longer generates the map\n";
    return false;
}

void report(const std::string& step, std::size_t multiplications, std::size_t slices,
            double cumulative_seconds, bool as_json) {
    if (as_json) {
        std::cout << "  {\"step\": \"" << step << "\", \"multiplications\": " << multiplications
                  << ", \"slices\": " << slices << ", \"cumulative_seconds\": "
                  << cumulative_seconds << "}\n";
    } else {
        std::cout << "  " << step << ": " << multiplications << " multiplications, " << slices
                  << " slices, " << cumulative_seconds << " s cumulative\n";
    }
}

/// The tool proper. main only turns a thrown refusal into a line.
int run(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: minimise-rank <tensor-file> [--steps 1|2|3] [--json]"
                     " [--emit-operators <stem>] [--max-memory 2G]"
                     " [--threads N]\n"
                     "  --emit-operators <stem>   write <stem>_{L,R,P}.sms, the encoding\n"
                     "                            operators, in the format PLinOpt reads\n"
                     "  --plateau N               allow N equal-cost steps\n"
                     "  --plateau-states N        distinct subspaces one crossing may visit,\n"
                     "                            from plateau_state_budget in tunables.conf\n"
                     "                            when this is not given. Never measured\n"
                     "  -s|--symmetry none|auto|matmul <n> <m> <k>\n"
                     "                            quotient step 3's pool by the map's own\n"
                     "                            automorphisms: one candidate per orbit\n";
        return cli::exit_status(cli::ExitCode::Usage);
    }

    std::string path = argv[1];
    int wanted_steps = 3;
    bool as_json = false;
    std::string operator_prefix;
    cli::Symmetry symmetry;
    std::size_t plateau_budget = 0;
    // The file first, so `--plateau-states` below can overwrite it. Until this
    // flag existed the number was a literal at the two call sites and no run had
    // ever tried another value.
    std::size_t plateau_states = cli::tunables().plateau_state_budget;
    for (int argument = 2; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (option == "--json") {
            as_json = true;
        } else if (option == "--steps" && argument + 1 < argc) {
            wanted_steps = std::stoi(argv[++argument]);
        } else if (option == "--plateau" && argument + 1 < argc) {
            plateau_budget = cli::parse_count(option, argv[++argument]);
        } else if (option == "--plateau-states" && argument + 1 < argc) {
            plateau_states = cli::parse_count(option, argv[++argument]);
        } else if (option == "--symmetry" || option == "-s") {
            symmetry = cli::parse_symmetry(argc, argv, argument);
        } else if (option == "--threads" && argument + 1 < argc) {
            bilinear_rank::set_worker_count(
                static_cast<std::size_t>(std::stoull(argv[++argument])));
        } else if (option == "--max-memory" && argument + 1 < argc) {
            bilinear_rank::set_memory_budget(cli::parse_memory_size(option, argv[++argument]));
        } else if (option == "--emit-operators" && argument + 1 < argc) {
            operator_prefix = argv[++argument];
        } else {
            std::cerr << "unrecognised option: " << option << "\n";
            return cli::exit_status(cli::ExitCode::Usage);
        }
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(path);
    const bilinear_rank::Field field(tensor.characteristic);
    const auto started = cli::Clock::now();

    std::cout << (as_json ? "[\n" : path + "\n");
    report("naive", linear_algebra::multiplication_count(field, tensor.slices), tensor.slices.size(), 0.0,
           as_json);

    std::vector<bilinear_rank::Matrix> current = bilinear_rank::minimum_weight_basis(field, tensor.slices);
    // Was 1 here and at every other verify below, which reads as a refusal. The
    // step ran and its own check rejected what it produced, so the news is that
    // a component is wrong, not that the map resists: Unverified, not No.
    if (!verify(field, current, tensor.slices, "step 1")) {
        return cli::exit_status(cli::ExitCode::Unverified);
    }
    if (as_json) std::cout << ",";
    report("step 1", linear_algebra::multiplication_count(field, current), current.size(),
           cli::elapsed_seconds(started), as_json);

    if (wanted_steps >= 2) {
        const std::vector<bilinear_rank::Matrix> own = bilinear_rank::rank_one_candidates(field, current);
        const std::vector<bilinear_rank::Matrix> shortlist =
            bilinear_rank::improving_candidates(field, current, own);
        current = bilinear_rank::minimise_rank(field, current, shortlist);
        if (!verify(field, current, tensor.slices, "step 2")) {
            return cli::exit_status(cli::ExitCode::Unverified);
        }
        if (as_json) std::cout << ",";
        report("step 2", linear_algebra::multiplication_count(field, current), current.size(),
               cli::elapsed_seconds(started), as_json);
    }

    if (wanted_steps >= 3) {
        // The pool addressed rather than built. `size()` is the same number
        // `all_rank_one_maps` would have reported and costs the two vector lists
        // rather than their product in matrices, so the line below is printed
        // before anything has decided whether the grid is needed at all.
        const bilinear_rank::RankOnePool pool(field, tensor.rows(), tensor.columns());
        std::cerr << "step 3 pool: " << pool.size() << " rank-one maps\n";

        if (symmetry.kind == cli::SymmetryKind::None) {
            const std::vector<bilinear_rank::Matrix> shortlist =
                bilinear_rank::improving_candidates(field, current, pool);
            std::cerr << "step 3 shortlist: " << shortlist.size() << "\n";
            current = bilinear_rank::minimise_rank(field, current, shortlist);
            if (plateau_budget > 0) {
                // Built here and nowhere else on this path. `cross_plateaus`
                // indexes a materialised pool, so --plateau still pays for the
                // grid; a run without it now never allocates one.
                const std::vector<bilinear_rank::Matrix> everything =
                    bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
                bilinear_rank::PlateauReport crossing;
                current = bilinear_rank::cross_plateaus(field, current, everything, {},
                                                        plateau_budget, plateau_states, &crossing);
                std::cerr << "plateau: " << crossing.improvements << " improvements, "
                          << crossing.sideways << " sideways, " << crossing.states
                          << " states, best " << crossing.best << "\n";
            }
        } else {
            // Materialised, deliberately. `pool_orbits.h` keys its orbit tables
            // by pool index and `minimise_rank_up_to_symmetry` reads `pool[i]`
            // back out of them, so this path wants the grid in the order
            // `all_rank_one_maps` builds it and an addressed pool would only
            // move the same allocation inside the loop.
            const std::vector<bilinear_rank::Matrix> everything =
                bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
            const std::vector<bilinear_rank::Automorphism> ambient =
                bilinear_rank::requested_ambient_group(field, tensor.slices, symmetry);
            std::cerr << "step 3 ambient generators: " << ambient.size() << "\n";

            bilinear_rank::OrbitReport orbits;
            current = bilinear_rank::minimise_rank_up_to_symmetry(field, current, everything, ambient,
                                                         &orbits);
            if (plateau_budget > 0) {
                bilinear_rank::PlateauReport crossing;
                current = bilinear_rank::cross_plateaus(field, current, everything, ambient,
                                                        plateau_budget, plateau_states, &crossing);
                std::cerr << "plateau: " << crossing.improvements << " improvements, "
                          << crossing.sideways << " sideways, " << crossing.states
                          << " states, best " << crossing.best << "\n";
            }
            for (std::size_t round = 0; round < orbits.orbits.size(); ++round) {
                std::cerr << "step 3 round " << round + 1 << ": stabiliser "
                          << orbits.stabiliser_size[round] << ", " << orbits.orbits[round]
                          << " orbits of " << orbits.pool << "\n";
            }
        }
        if (!verify(field, current, tensor.slices, "step 3")) {
            return cli::exit_status(cli::ExitCode::Unverified);
        }
        if (as_json) std::cout << ",";
        report("step 3", linear_algebra::multiplication_count(field, current), current.size(),
               cli::elapsed_seconds(started), as_json);
    }

    // The multiplication count is not the deliverable: the algorithm is.
    // Recovering it also emits the operators the sparsification then works on,
    // which is what joins the two halves of this repository into one pipeline.
    const std::vector<bilinear_rank::Matrix> products =
        bilinear_rank::rank_one_candidates(field, current);
    bilinear_rank::Algorithm algorithm;
    if (!bilinear_rank::recovers_map(field, tensor.slices, products, algorithm)) {
        std::cerr << "FAILED: the decomposition did not turn back into an algorithm "
                     "that computes the map\n";
        // The same failure as a rejected verify, one stage later, so the same
        // code: was 1, and an answer that fails its own check is Unverified.
        return cli::exit_status(cli::ExitCode::Unverified);
    }
    // What is left to win. A heuristic cannot say whether its answer is optimal,
    // but the flattenings say how much room there is underneath it, and a gap of
    // zero means the run is finished and nothing exponential need follow it.
    const std::size_t bound = bilinear_rank::flattening_floor(field, tensor.slices);
    std::cerr << "algorithm: " << bilinear_rank::require_bound_consistent(algorithm.product_count(), bound)
              << ", L is "
              << algorithm.left.rows() << "x" << algorithm.left.columns() << ", R is "
              << algorithm.right.rows() << "x" << algorithm.right.columns() << ", P is "
              << algorithm.decode.rows() << "x" << algorithm.decode.columns() << "\n";

    if (!operator_prefix.empty()) {
        // The triple ⟨L, R, P⟩ under one stem, in SMS, because that is what the
        // surrounding ecosystem takes: PLinOpt's checkers are invoked as
        // `PMchecker stem_{L,R,P}.sms -q p`, so the stem and the three suffixes
        // are the interface, not a local naming choice. P was recovered all
        // along and never written, which left the algorithm unverifiable by
        // anything outside this repository.
        const std::string origin = "Encoding operator recovered from " + path +
                                   "\nby minimise-rank, " +
                                   std::to_string(algorithm.product_count()) + " products, over GF(" +
                                   std::to_string(tensor.characteristic) + ").";
        linear_algebra::write_sms_file(operator_prefix + "_L.sms",
                                       origin + " Left operand.", algorithm.left);
        linear_algebra::write_sms_file(operator_prefix + "_R.sms",
                                       origin + " Right operand.", algorithm.right);
        linear_algebra::write_sms_file(operator_prefix + "_P.sms",
                                       origin + " Combines the products into the outputs.",
                                       algorithm.decode);
        std::cerr << "wrote " << operator_prefix << "_{L,R,P}.sms\n";
    }

    if (as_json) std::cout << "]\n";
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line, or a line of tunables.conf, that could not
        // be read: the run never started, so Usage rather than Error.
        std::cerr << "minimise-rank: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        // An unreadable file, or a run that would not fit the memory budget.
        // Reported as a line, not as a terminate. Was 1, which said something
        // about the map when the heuristic had not run on it at all.
        std::cerr << "minimise-rank: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }
}
