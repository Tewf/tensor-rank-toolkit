/// Move a decomposition rather than build one: the flip graph, over any GF(p).
///
/// Every other command here assembles an algorithm out of a pool of rank-one
/// maps. This one starts from a scheme somebody already holds and walks it: the
/// naive algorithm, which always exists, or under `--from` the heuristic's
/// answer, which is much closer to the floor. It proves nothing: each seed's
/// result is checked against the map it must compute before its count is
/// printed, and a count from an unchecked scheme is never printed at all.
#include <stdexcept>
#include <string>
#include <vector>

#include "card_failure_note.h"
#include "algorithm_recovery.h"
#include "arguments.h"
#include "candidate_pool.h"
#include "exit_code.h"
#include "fewest_products.h"
#include "flip_graph.h"
#include "memory_budget.h"
#include "minimise_rank.h"
#include "parallel.h"
#include "report.h"
#include "tensor_file.h"
#include "timing.h"

namespace {

void usage() {
    cli::note() << "usage: walk-scheme <tensor-file> [--flips N] [--seeds N] [--from k]\n"
                   "                   [--threads N] [--max-memory N] [--help]\n"
                   "\n"
                   "  --flips N   flips per seed, 20000 by default. --steps is the older\n"
                   "              spelling and still works; it means pipeline stages in\n"
                   "              minimise-rank, which is why this one is not called that\n"
                   "  --seeds N   independent walks, 8 by default; each is reproducible\n"
                   "              from its own seed number\n"
                   "  --threads N N workers, 0 for every core, 1 by default. The seeds are\n"
                   "              independent walks, so this changes the wall clock and\n"
                   "              nothing else: same seeds, same schemes, same output\n"
                   "  --from k    walk from the heuristic's k-product scheme rather than\n"
                   "              from the naive algorithm. The heuristic has to reach k\n"
                   "              or fewer or the run refuses, because a starting point\n"
                   "              nobody holds is not a starting point\n"
                   "  --max-memory N\n"
                   "              bytes one bulk allocation may take, 2G by default. --from\n"
                   "              runs the heuristic first, whose span table is p^dim, so\n"
                   "              this is what refuses a shape the machine cannot hold\n"
                   "  --help      print this and stop, as exit 2";
}

/// The heuristic's answer, which is what `--from` walks away from: the three
/// steps `minimise-rank` runs by default, and nothing exponential.
///
/// This is the join between the two commands. The heuristic descends fast and
/// then stops on a plateau it cannot cross, and the walk is the only thing here
/// that moves sideways, so neither reaches what the pair reaches.
std::vector<bilinear_rank::Matrix> heuristic_scheme(const bilinear_rank::Field& field,
                                                   const formats::Tensor& tensor) {
    const std::vector<bilinear_rank::Matrix> current =
        bilinear_rank::descend_from_own_basis(field, tensor.slices);
    // Addressed, not built. This is one forward scan that keeps no index once it
    // is past, so the pool never has to exist at once: the two vector lists cost
    // `p^rows + p^columns` against the grid's product, and only the shortlist
    // that comes back is held.
    const bilinear_rank::RankOnePool pool(field, tensor.rows(), tensor.columns());
    return bilinear_rank::minimise_rank(
        field, current, bilinear_rank::improving_candidates(field, current, pool));
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
    std::size_t flips = 20000;
    std::size_t seeds = 8;
    long long from = -1;
    // Walked by `cli/arguments.h` rather than by hand. This loop's `std::stoul`
    // was the worst of the copies: `--flips abc` left as 5 printing a bare
    // `stoul`, with no flag, no word and not even the program's name in front of
    // it, since the handler below had none either.
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--flips", "--steps")) {
            flips = arguments.count();
        } else if (arguments.is("--seeds")) {
            seeds = arguments.count();
        } else if (arguments.is("--threads")) {
            run_limits::set_worker_count(arguments.count());
        } else if (arguments.is("--max-memory")) {
            run_limits::set_memory_budget(arguments.memory_size());
        } else if (arguments.is("--from")) {
            from = arguments.whole_number();
        } else {
            arguments.refuse();
        }
    }
    // No file named, and nothing here reads a scheme on stdin.
    if (arguments.no_file_named()) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string path = arguments.filename();

    const formats::Tensor tensor = formats::read_tensor_file(path);
    const bilinear_rank::Field field(tensor.characteristic);

    bilinear_rank::Algorithm naive;
    if (!bilinear_rank::recovers_map(
            field, tensor.slices, bilinear_rank::rank_one_candidates(field, tensor.slices), naive)) {
        cli::note() << "no naive algorithm for this map, so nothing to walk from";
        // Was 1. Nothing was walked and nothing refuted: without a starting
        // scheme the tool cannot begin, which is Error and not an answer.
        return cli::exit_status(cli::ExitCode::Error);
    }

    bilinear_rank::Scheme start = bilinear_rank::scheme_of(naive);
    cli::result() << "GF(" << tensor.characteristic << "), naive scheme: " << start.size()
                  << " products\n";

    if (from >= 0) {
        bilinear_rank::Algorithm reduced;
        const std::vector<bilinear_rank::Matrix> heuristic = heuristic_scheme(field, tensor);
        if (!bilinear_rank::recovers_map(field, tensor.slices,
                                        bilinear_rank::rank_one_candidates(field, heuristic),
                                        reduced)) {
            cli::note() << "the heuristic's result did not turn back into an algorithm, so "
                           "there is nothing to walk from";
            // Was 1. The heuristic produced a result and recovery rejected it,
            // which is an answer failing its own check: Unverified.
            return cli::exit_status(cli::ExitCode::Unverified);
        }
        if (reduced.product_count() > static_cast<std::size_t>(from)) {
            cli::note() << "the heuristic reached " << reduced.product_count()
                        << " products, not " << from << ", so there is no " << from
                        << "-product scheme to walk from";
            // Was 1, which reads as a refutation of a k-product scheme. Nothing
            // of the sort was shown: --from k asked for a starting point this
            // heuristic does not hand over, so the argument is the problem.
            return cli::exit_status(cli::ExitCode::Usage);
        }
        start = bilinear_rank::scheme_of(reduced);
        cli::result() << "heuristic scheme: " << start.size()
                      << " products, walking from there\n";
    }

    const cli::Clock::time_point started = cli::Clock::now();

    // Walk every seed, then reduce over them in seed order.
    //
    // The seeds are independent: each is `mt19937_64(seed)` and reads a `start`
    // and a field that nobody writes, so a walk is the same walk whichever worker
    // runs it and the answers are bit-identical at any thread count. That is what
    // makes this the one search here that parallelises without an argument about
    // node counts: there is no shared budget, no early exit and no race, only
    // eight equal-cost walks and a minimum over them.
    //
    // The reduction stays sequential and stays second, because `best` carries
    // between seeds: a seed prints only if it beats every *earlier* seed, and
    // whether a scheme is verified at all depends on that. Reducing as the walks
    // landed would make the printed lines depend on which worker finished first.
    struct Walked {
        bilinear_rank::Scheme scheme;
        bilinear_rank::FlipReport report;
        double seconds = 0.0;
    };
    std::vector<Walked> walks(seeds);
    run_limits::parallel_for(seeds, [&](std::size_t offset) {
        const cli::Clock::time_point walk_started = cli::Clock::now();
        walks[offset].scheme = bilinear_rank::random_flip_walk(field, start, flips, offset + 1,
                                                              &walks[offset].report);
        walks[offset].seconds = cli::elapsed_seconds(walk_started);
    });

    std::size_t best = start.size();
    for (std::size_t seed = 1; seed <= seeds; ++seed) {
        const Walked& walked = walks[seed - 1];
        if (walked.scheme.size() >= best) continue;
        if (!computes(field, tensor.slices, walked.scheme)) {
            cli::result() << "  seed " << seed << ": " << walked.scheme.size()
                          << " products, DISCARDED, does not compute the map\n";
            continue;
        }
        best = walked.scheme.size();
        // This seed's own walk, not the elapsed run: with workers the two differ,
        // and a cumulative figure would say when a worker happened to be reached.
        cli::result() << "  seed " << seed << ": " << best << " products after "
                      << walked.report.flips << " flips and " << walked.report.reductions
                      << " reductions, " << walked.seconds << " s\n";
    }
    const std::size_t bound = bilinear_rank::flattening_floor(field, tensor.slices);
    cli::result() << "best over " << seeds << " seeds: "
                  << bilinear_rank::require_bound_consistent(best, bound) << "\n";
    // The wall clock for the whole walk, on stderr where a non-result belongs,
    // following `decide-rank-by-pencil`. With workers the per-seed figures above
    // no longer sum to this, which is the reason it is printed at all: the number
    // the write-ups quote for a walk is this one.
    cli::note() << cli::elapsed_seconds(started) << " s, " << run_limits::worker_count()
                << " worker(s)";
    bilinear_rank::note_if_the_card_failed();
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line that could not be read: the run never
        // started, so Usage rather than Error. Without this catch every refusal
        // the walker makes would leave as 5, and a bad flag is not a tool that
        // could not run.
        cli::note() << "walk-scheme: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& failure) {
        // Was 1, which reads as a refusal about the map. An unreadable file is
        // a tool that could not run, and the walk refutes nothing in any case.
        // Named, because a bare message told a reader which of the commands
        // it came from nothing at all.
        cli::note() << "walk-scheme: " << failure.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
