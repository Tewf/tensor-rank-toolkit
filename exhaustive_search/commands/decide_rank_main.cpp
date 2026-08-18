/// Decide, rather than improve: is there an algorithm with exactly k products?
///
/// The heuristic in `minimise-rank` answers "can this be made better". This
/// answers "is there one this small", and a negative answer means no such
/// algorithm exists, provided the search ran to exhaustion, which is why the
/// node budget is reported on every line.
#include <iostream>
#include <stdexcept>
#include <string>

#include "algorithm_recovery.h"
#include "candidate_pool.h"
#include "dense_matrix_file.h"
#include "exhaustive_search.h"
#include "fewest_products.h"
#include "group_construction.h"
#include "memory_budget.h"
#include "minimise_rank.h"
#include "orbit_search.h"
#include "parallel.h"
#include "requested_group.h"
#include "size_argument.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "timing.h"

namespace {

void usage() {
    std::cerr << "usage: decide-rank <tensor-file> [--target k] [--anchor map|heuristic]\n"
                 "                   [--node-limit N] [--bottom-up] [--max-memory 2G]\n"
                 "                   [--threads N]   N workers, 0 for every core, 1 by default\n"
                 "                   [-s|--symmetry none|auto|matmul <n> <m> <k>]\n"
                 "\n"
                 "  --anchor map        search from the map itself (default): the answer is\n"
                 "                      the true minimum, and the search is exponential\n"
                 "  --anchor heuristic  run the heuristic first and search from its result:\n"
                 "                      far cheaper, but the answer is the minimum only\n"
                 "                      among algorithms containing that subspace\n";
}

/// The tool proper. main only turns a thrown refusal into a line.
int run(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }

    const std::string path = argv[1];
    long long target = -1;
    bool anchor_on_heuristic = false;
    bool bottom_up = false;
    std::size_t node_limit = 5'000'000;
    cli::Symmetry symmetry;

    for (int argument = 2; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (option == "--target" && argument + 1 < argc) {
            target = std::stoll(argv[++argument]);
        } else if (option == "--anchor" && argument + 1 < argc) {
            anchor_on_heuristic = (std::string(argv[++argument]) == "heuristic");
        } else if (option == "--symmetry" || option == "-s") {
            symmetry = cli::parse_symmetry(argc, argv, argument);
        } else if (option == "--threads" && argument + 1 < argc) {
            bilinear_rank::set_worker_count(
                static_cast<std::size_t>(std::stoull(argv[++argument])));
        } else if (option == "--max-memory" && argument + 1 < argc) {
            bilinear_rank::set_memory_budget(cli::parse_size(argv[++argument]));
        } else if (option == "--node-limit" && argument + 1 < argc) {
            node_limit = static_cast<std::size_t>(std::stoull(argv[++argument]));
        } else if (option == "--bottom-up") {
            bottom_up = true;
        } else {
            usage();
            return 2;
        }
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(path);
    const bilinear_rank::Field field(tensor.characteristic);

    // Cheap next to a search, and it settles every k below it. The sweep starts
    // here rather than at the span dimension, and a target underneath it is
    // refused without a search: that refusal is a proof, not a budget expiring.
    const std::size_t bound = bilinear_rank::flattening_floor(field, tensor.slices);
    std::cout << path << "\n  rank bound: rank is at least " << bound << "\n";
    if (target >= 0 && static_cast<std::size_t>(target) < bound) {
        std::cout << "  NO: there is no algorithm with " << target
                  << " products, which the polynomial bounds already refute.\n";
        return 1;
    }

    std::vector<bilinear_rank::Matrix> anchor = tensor.slices;
    if (anchor_on_heuristic) {
        anchor = bilinear_rank::descend_from_own_basis(field, tensor.slices);
        std::cout << "anchored on the heuristic: " << anchor.size() << " slices, "
                  << linear_algebra::multiplication_count(field, anchor) << " multiplications\n";
    }

    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
    std::cout << "  pool: " << pool.size() << " rank-one maps of shape " << tensor.rows()
              << "x" << tensor.columns() << "\n";

    bilinear_rank::SearchBudget budget{node_limit};
    std::vector<bilinear_rank::Matrix> products;
    const auto started = cli::Clock::now();

    bool found = false;
    if (bottom_up) {
        found = bilinear_rank::fewest_products_from_scratch(field, tensor.slices, pool, budget, products);
    } else if (target >= 0 && symmetry.kind != cli::SymmetryKind::None) {
        // The search needs a group that stabilises what it is searching from, and
        // `anchor` is not always the map: under `--anchor heuristic` it is the
        // heuristic's subspace, whose stabiliser is a different group.
        const std::vector<bilinear_rank::Automorphism> generators = bilinear_rank::stabiliser_of(
            field, anchor, bilinear_rank::requested_ambient_group(field, tensor.slices, symmetry));
        std::cout << "  quotienting by " << generators.size() << " generators\n";
        found = bilinear_rank::expand_subspace_up_to_symmetry(field, anchor, pool, generators,
                                                     static_cast<std::size_t>(target), budget,
                                                     products);
    } else if (target >= 0) {
        found = bilinear_rank::expand_subspace(field, anchor, pool, 0,
                                               static_cast<std::size_t>(target), budget, products);
    } else {
        found = bilinear_rank::fewest_products_by_sweep(field, anchor, pool, budget, products);
    }
    const double seconds = cli::elapsed_seconds(started);

    std::cout << "  " << budget.nodes_visited << " nodes in " << seconds << " s\n";

    if (found) {
        std::cout << "  FOUND: " << bilinear_rank::require_bound_consistent(products.size(), bound) << "\n";
        bilinear_rank::Algorithm algorithm;
        if (!bilinear_rank::recovers_map(field, tensor.slices, products, algorithm)) {
            std::cerr << "FAILED: those products do not compute the map\n";
            return 1;
        }
        std::cout << "  verified: they compute the map\n";
        return 0;
    }

    if (!budget.exhausted) {
        std::cout << "  GAVE UP: the node limit was reached, so nothing is decided.\n"
                     "           Raise --node-limit to search further.\n";
        return 2;
    }
    if (target >= 0) {
        std::cout << "  NO: there is no algorithm with " << target << " products"
                  << (anchor_on_heuristic ? " containing the heuristic's subspace" : "")
                  << ". The search was exhaustive.\n";
    } else {
        std::cout << "  NO decomposition found in the searched range.\n";
    }
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& problem) {
        // A refusal is a result: an unreadable file, or a run that would not
        // fit the memory budget. Reported as a line, not as a terminate.
        std::cerr << "decide-rank: " << problem.what() << "\n";
        return 1;
    }
}
