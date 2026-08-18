/// Move a decomposition rather than build one: the flip graph, over any GF(p).
///
/// Every other command here assembles an algorithm out of a pool of rank-one
/// maps. This one starts from a scheme somebody already holds and walks it: the
/// naive algorithm, which always exists, or under `--from` the heuristic's
/// answer, which is much closer to the floor. It proves nothing: each seed's
/// result is checked against the map it must compute before its count is
/// printed, and a count from an unchecked scheme is never printed at all.
#include <iostream>
#include <stdexcept>
#include <string>

#include "algorithm_recovery.h"
#include "candidate_pool.h"
#include "fewest_products.h"
#include "flip_graph.h"
#include "minimise_rank.h"
#include "tensor_file.h"
#include "timing.h"

namespace {

void usage() {
    std::cerr << "usage: walk-scheme <tensor-file> [--flips N] [--seeds N] [--from k]\n"
                 "\n"
                 "  --flips N   flips per seed, 20000 by default. --steps is the older\n"
                 "              spelling and still works; it means pipeline stages in\n"
                 "              minimise-rank, which is why this one is not called that\n"
                 "  --seeds N   independent walks, 8 by default; each is reproducible\n"
                 "              from its own seed number\n"
                 "  --from k    walk from the heuristic's k-product scheme rather than\n"
                 "              from the naive algorithm. The heuristic has to reach k\n"
                 "              or fewer or the run refuses, because a starting point\n"
                 "              nobody holds is not a starting point\n";
}

/// The heuristic's answer, which is what `--from` walks away from: the three
/// steps `minimise-rank` runs by default, and nothing exponential.
///
/// This is the join between the two commands. The heuristic descends fast and
/// then stops on a plateau it cannot cross, and the walk is the only thing here
/// that moves sideways, so neither reaches what the pair reaches.
std::vector<bilinear_rank::Matrix> heuristic_scheme(const bilinear_rank::Field& field,
                                                   const linear_algebra::Tensor& tensor) {
    const std::vector<bilinear_rank::Matrix> current =
        bilinear_rank::descend_from_own_basis(field, tensor.slices);
    const std::vector<bilinear_rank::Matrix> everything =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
    return bilinear_rank::minimise_rank(
        field, current, bilinear_rank::improving_candidates(field, current, everything));
}

/// Does this scheme still compute the map it started from? Asked of every
/// result, because a scheme that does not is not a cheaper algorithm.
bool computes(const bilinear_rank::Field& field,
              const std::vector<bilinear_rank::Matrix>& target,
              const bilinear_rank::Scheme& scheme) {
    const bilinear_rank::Algorithm algorithm = bilinear_rank::algorithm_of(scheme);
    if (algorithm.product_count() == 0) return false;
    bilinear_rank::Algorithm recovered;
    return bilinear_rank::recovers_map(
        field, target, bilinear_rank::encoded_products(field, algorithm.left, algorithm.right),
        recovered);
}

int run(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }

    std::size_t flips = 20000;
    std::size_t seeds = 8;
    long long from = -1;
    for (int argument = 2; argument < argc; ++argument) {
        const std::string flag = argv[argument];
        if ((flag == "--flips" || flag == "--steps") && argument + 1 < argc) {
            flips = std::stoul(argv[++argument]);
        } else if (flag == "--seeds" && argument + 1 < argc) {
            seeds = std::stoul(argv[++argument]);
        } else if (flag == "--from" && argument + 1 < argc) {
            from = std::stoll(argv[++argument]);
        } else {
            usage();
            return 2;
        }
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(argv[1]);
    const bilinear_rank::Field field(tensor.characteristic);

    bilinear_rank::Algorithm naive;
    if (!bilinear_rank::recovers_map(
            field, tensor.slices, bilinear_rank::rank_one_candidates(field, tensor.slices), naive)) {
        std::cerr << "no naive algorithm for this map, so nothing to walk from\n";
        return 1;
    }

    bilinear_rank::Scheme start = bilinear_rank::scheme_of(naive);
    std::cout << "GF(" << tensor.characteristic << "), naive scheme: " << start.size()
              << " products\n";

    if (from >= 0) {
        bilinear_rank::Algorithm reduced;
        const std::vector<bilinear_rank::Matrix> heuristic = heuristic_scheme(field, tensor);
        if (!bilinear_rank::recovers_map(field, tensor.slices,
                                        bilinear_rank::rank_one_candidates(field, heuristic),
                                        reduced)) {
            std::cerr << "the heuristic's result did not turn back into an algorithm, so there "
                         "is nothing to walk from\n";
            return 1;
        }
        if (reduced.product_count() > static_cast<std::size_t>(from)) {
            std::cerr << "the heuristic reached " << reduced.product_count() << " products, not "
                      << from << ", so there is no " << from << "-product scheme to walk from\n";
            return 1;
        }
        start = bilinear_rank::scheme_of(reduced);
        std::cout << "heuristic scheme: " << start.size() << " products, walking from there\n";
    }

    const cli::Clock::time_point started = cli::Clock::now();
    std::size_t best = start.size();
    for (std::size_t seed = 1; seed <= seeds; ++seed) {
        bilinear_rank::FlipReport report;
        const bilinear_rank::Scheme walked = bilinear_rank::walk(field, start, flips, seed, &report);
        if (walked.size() >= best) continue;
        if (!computes(field, tensor.slices, walked)) {
            std::cout << "  seed " << seed << ": " << walked.size()
                      << " products, DISCARDED, does not compute the map\n";
            continue;
        }
        best = walked.size();
        std::cout << "  seed " << seed << ": " << best << " products after " << report.flips
                  << " flips and " << report.reductions << " reductions, " << cli::elapsed_seconds(started)
                  << " s\n";
    }
    const std::size_t bound = bilinear_rank::starting_target(field, tensor.slices);
    std::cout << "best over " << seeds << " seeds: " << bilinear_rank::gap_report(best, bound)
              << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << "\n";
        return 1;
    }
}
