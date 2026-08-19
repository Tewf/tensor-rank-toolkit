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
#include "arguments.h"
#include "candidate_pool.h"
#include "dense_matrix_file.h"
#include "exhaustive_search.h"
#include "exit_code.h"
#include "fewest_products.h"
#include "gf2_leaf.h"
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
#include "tunables.h"

namespace {

void usage() {
    std::cerr << "usage: decide-rank <tensor-file> [--target k] [--anchor map|heuristic]\n"
                 "                   [--node-limit N] [--leaf-limit N] [--bottom-up]\n"
                 "                   [--max-memory 2G] [--general-leaf]\n"
                 "                   [--threads N]   N workers, 0 for every core, 1 by default\n"
                 "                   [-s|--symmetry none|auto|matmul <n> <m> <k>]\n"
                 "\n"
                 "  --anchor map        search from the map itself (default): the answer is\n"
                 "                      the true minimum, and the search is exponential\n"
                 "  --anchor heuristic  run the heuristic first and search from its result:\n"
                 "                      far cheaper, but the answer is the minimum only\n"
                 "                      among algorithms containing that subspace\n"
                 "  --node-limit N      nodes the search may visit, from search_node_limit\n"
                 "                      in tunables.conf when this is not given. Reaching\n"
                 "                      it is exit 3 and proves nothing either way\n"
                 "  --leaf-limit N      elements one leaf may examine, from search_leaf_limit.\n"
                 "                      The node limit bounds how many leaves are reached and\n"
                 "                      nothing inside one; this is what bounds one. Reaching\n"
                 "                      it is exit 3 too, and proves nothing either way\n"
                 "  --general-leaf      answer every leaf by the general field path, even over\n"
                 "                      GF(2) where the bit-packed one applies. Same tree, same\n"
                 "                      nodes, same answer, and slower: it is here so the two\n"
                 "                      can be timed on one question rather than on two\n";
}

/// The tool proper. main only turns a thrown refusal into a line.
int run(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    const std::string path = argv[1];
    long long target = -1;
    bool anchor_on_heuristic = false;
    bool bottom_up = false;
    // The file first, so that `--node-limit` below can overwrite it: a flag that
    // was given always wins over tunables.conf, and one that was not leaves the
    // file's number standing.
    std::size_t node_limit = cli::tunables().search_node_limit;
    std::size_t leaf_limit = cli::tunables().search_leaf_limit;
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
            bilinear_rank::set_memory_budget(cli::parse_memory_size(option, argv[++argument]));
        } else if (option == "--node-limit" && argument + 1 < argc) {
            node_limit = cli::parse_count(option, argv[++argument]);
        } else if (option == "--leaf-limit" && argument + 1 < argc) {
            leaf_limit = cli::parse_count(option, argv[++argument]);
        } else if (option == "--bottom-up") {
            bottom_up = true;
        } else if (option == "--general-leaf") {
            bilinear_rank::set_gf2_leaf_offered(false);
        } else {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
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
        return cli::exit_status(cli::ExitCode::No);
    }

    std::vector<bilinear_rank::Matrix> anchor = tensor.slices;
    if (anchor_on_heuristic) {
        anchor = bilinear_rank::descend_from_own_basis(field, tensor.slices);
        std::cout << "anchored on the heuristic: " << anchor.size() << " slices, "
                  << linear_algebra::multiplication_count(field, anchor) << " multiplications\n";
    }

    // Materialise the pool where it fits and address it where it does not.
    //
    // The recursion carries an index down and resumes from it, so an addressed
    // pool rebuilds a map once per node that reaches it rather than once per run.
    // That is a real cost and it is why the materialised pool stays the default.
    // It is also plainly the right trade when the alternative is refusing to
    // start: at `⟨4,4,4⟩` the pool is 4.3e9 maps and 8.2 TiB, and a slower search
    // beats no search. This is the odometer of `[yang2025]`, whose whole
    // difference from `[bdez2012]` Algorithm 1 is that it never holds the pool.
    const bilinear_rank::RankOnePool addressed(field, tensor.rows(), tensor.columns());
    const bool fits = bilinear_rank::bytes_per_matrix(tensor.rows() * tensor.columns()) <=
                      bilinear_rank::memory_budget() / addressed.size();
    std::cout << "  pool: " << addressed.size() << " rank-one maps of shape " << tensor.rows()
              << "x" << tensor.columns() << (fits ? ", materialised" : ", addressed by index")
              << "\n";

    // Which leaf test answered, printed for the same reason the pool line is:
    // a timing whose route is not on the line beside it is a timing of an
    // unknown thing, and `--general-leaf` is here precisely to move this.
    std::cout << "  leaf: "
              << (bilinear_rank::gf2_leaf_applies(field, tensor.columns())
                      ? "GF(2), one bit per entry"
                      : "general field path")
              << "\n";

    std::vector<bilinear_rank::Matrix> pool;
    if (fits) pool = bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    bilinear_rank::SearchBudget budget{node_limit, leaf_limit};
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
    } else if (target >= 0 && !fits) {
        // Only the plain route has an addressed form. The quotiented search keys
        // orbit tables by pool position and the sweeps index down a recursion in
        // parallel, so neither is converted, and a `--symmetry` or sweep request
        // on a shape this large is refused above rather than answered wrongly.
        found = bilinear_rank::expand_subspace(field, anchor, addressed, 0,
                                              static_cast<std::size_t>(target), budget, products);
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
            // Was 1, which reads as "no such algorithm". The search did find
            // one and then its own check threw it out, so a component here is
            // wrong: that is Unverified, and it is worse news than a refusal.
            return cli::exit_status(cli::ExitCode::Unverified);
        }
        std::cout << "  verified: they compute the map\n";
        return cli::exit_status(cli::ExitCode::Yes);
    }

    if (!budget.exhausted) {
        const bool leaf = budget.leaf_abandoned.load();
        std::cout << "  GAVE UP: the " << (leaf ? "leaf" : "node")
                  << " limit was reached, so nothing is decided.\n"
                     "           Raise --"
                  << (leaf ? "leaf" : "node") << "-limit to search further.\n";
        // Was 2, the same code the two usage errors above return, so a script
        // could not tell a mistyped flag from a search that ran and gave up.
        // A spent budget decides nothing, which is exactly what 3 is for.
        return cli::exit_status(cli::ExitCode::Undecided);
    }
    if (target >= 0) {
        std::cout << "  NO: there is no algorithm with " << target << " products"
                  << (anchor_on_heuristic ? " containing the heuristic's subspace" : "")
                  << ". The search was exhaustive.\n";
    } else {
        std::cout << "  NO decomposition found in the searched range.\n";
    }
    return cli::exit_status(cli::ExitCode::No);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line, or a line of tunables.conf, that could not
        // be read. The run never started, so it is Usage and not Error, which is
        // the distinction `arguments.h` exists to keep.
        std::cerr << "decide-rank: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        // An unreadable file, or a run that would not fit the memory budget.
        // Reported as a line, not as a terminate. Was 1, which claimed no such
        // algorithm exists on the strength of a search that never started.
        std::cerr << "decide-rank: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }
}
