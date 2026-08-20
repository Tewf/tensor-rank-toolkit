/// Decide, rather than improve: is there an algorithm with exactly k products?
///
/// The heuristic in `minimise-rank` answers "can this be made better". This
/// answers "is there one this small", and a negative answer means no such
/// algorithm exists, provided the search ran to exhaustion, which is why the
/// node budget is reported on every line.
#include <stdexcept>
#include <limits>
#include <string>

#include "algorithm_recovery.h"
#include "arguments.h"
#include "candidate_pool.h"
#include "dense_matrix_file.h"
#include "device.h"
#include "exhaustive_search.h"
#include "exit_code.h"
#include "fewest_products.h"
#include "gf2_leaf.h"
#include "group_construction.h"
#include "memory_budget.h"
#include "minimise_rank.h"
#include "orbit_search.h"
#include "parallel.h"
#include "report.h"
#include "requested_group.h"
#include "size_argument.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "timing.h"
#include "tunables.h"

namespace {

void usage() {
    cli::note() << "usage: decide-rank <tensor-file> [--target k] [--anchor map|heuristic]\n"
                   "                   [--node-limit N] [--leaf-limit N]\n"
                   "                   [--max-memory 2G] [--general-leaf] [--help]\n"
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
                   "                      can be timed on one question rather than on two\n"
                   "  --help              print this and stop, as exit 2";
}

/// The tool proper. main only turns a thrown refusal into a line.
int run(int argc, char** argv) {
    long long target = -1;
    bool anchor_on_heuristic = false;
    // The file first, so that `--node-limit` below can overwrite it: a flag that
    // was given always wins over tunables.conf, and one that was not leaves the
    // file's number standing.
    // Which processor a bulk question would go to. Applied before anything is
    // measured, and reported below, because a timing whose device is not on the
    // line beside it is a timing of an unknown thing. Nothing is compiled in
    // behind `gpu` yet, so this resolves to the host and says so.
    {
        std::string unrecognised;
        if (!run_limits::set_device_order(cli::tunables().device_order, unrecognised)) {
            throw cli::ArgumentError("tunables.conf device_order: no device is called '" +
                                     unrecognised + "'");
        }
        run_limits::set_launch_floor(cli::tunables().device_launch_floor);
    }

    std::size_t node_limit = cli::tunables().search_node_limit;
    std::size_t leaf_limit = cli::tunables().search_leaf_limit;
    cli::Symmetry symmetry;

    // Walked by `cli/arguments.h` rather than by hand, so `--target abc` names the
    // flag and the word instead of leaving as 5 saying `stoll`, and `--target`
    // with nothing after it is a missing value rather than a misspelt flag.
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--target")) {
            target = arguments.whole_number();
        } else if (arguments.is("--anchor")) {
            anchor_on_heuristic = (arguments.text() == "heuristic");
        } else if (arguments.is("--symmetry", "-s")) {
            symmetry = arguments.parsed_by(cli::parse_symmetry);
        } else if (arguments.is("--threads")) {
            bilinear_rank::set_worker_count(arguments.count());
        } else if (arguments.is("--max-memory")) {
            bilinear_rank::set_memory_budget(arguments.memory_size());
        } else if (arguments.is("--node-limit")) {
            node_limit = arguments.count();
        } else if (arguments.is("--leaf-limit")) {
            leaf_limit = arguments.count();
        } else if (arguments.is("--general-leaf")) {
            bilinear_rank::set_gf2_leaf_offered(false);
        } else {
            arguments.refuse();
        }
    }
    // No file named, and nothing here reads a map on stdin.
    if (arguments.reads_stdin()) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string path = arguments.filename();

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(path);
    const bilinear_rank::Field field(tensor.characteristic);

    // Cheap next to a search, and it settles every k below it. The sweep starts
    // here rather than at the span dimension, and a target underneath it is
    // refused without a search: that refusal is a proof, not a budget expiring.
    const std::size_t bound = bilinear_rank::flattening_floor(field, tensor.slices);
    cli::result() << path << "\n  rank bound: rank is at least " << bound << "\n";
    if (target >= 0 && static_cast<std::size_t>(target) < bound) {
        cli::result() << "  NO: there is no algorithm with " << target
                      << " products, which the polynomial bounds already refute.\n";
        return cli::exit_status(cli::ExitCode::No);
    }

    std::vector<bilinear_rank::Matrix> anchor = tensor.slices;
    if (anchor_on_heuristic) {
        anchor = bilinear_rank::descend_from_own_basis(field, tensor.slices);
        cli::result() << "anchored on the heuristic: " << anchor.size() << " slices, "
                      << linear_algebra::multiplication_count(field, anchor)
                      << " multiplications\n";
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
    cli::result() << "  pool: " << addressed.size() << " rank-one maps of shape "
                  << tensor.rows() << "x" << tensor.columns()
                  << (fits ? ", materialised" : ", addressed by index") << "\n";

    // Which leaf test answered, printed for the same reason the pool line is:
    // a timing whose route is not on the line beside it is a timing of an
    // unknown thing, and `--general-leaf` is here precisely to move this.
    cli::result() << "  leaf: "
                  << (bilinear_rank::gf2_leaf_applies(field, tensor.columns())
                          ? "GF(2), one bit per entry"
                          : "general field path")
                  << "\n";

    // The ranking against what this build can reach, rather than a per-leaf
    // choice: a leaf picks its own route by size and there are many of them, so
    // what a reader needs is which devices were on the table at all.
    cli::result() << "  device: " << run_limits::name_of(run_limits::chosen_device(
                                        std::numeric_limits<std::size_t>::max()))
                  << (run_limits::available(run_limits::Device::Gpu)
                          ? ""
                          : " (no gpu backend compiled in)")
                  << "\n";

    std::vector<bilinear_rank::Matrix> pool;
    if (fits) pool = bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());

    // Only the plain search with an explicit target has an addressed form. The
    // other routes read the materialised `pool`, which is **empty** when it did
    // not fit, and an empty pool is not a small pool: the quotiented search
    // returned "NO ... exhaustive" for 47 products on `<4,4,4>` after one node,
    // which AlphaTensor exhibits, so it was a false refutation and exit 1. The
    // sweep gave up rather than answering, which was luck and not a defence.
    //
    // Refused here, where the comment below always claimed it was.
    if (!fits) {
        const char* why = nullptr;
        if (target < 0) {
            why = "a sweep with no --target";
        }
        if (why != nullptr) {
            throw std::runtime_error(
                std::string("the pool of this shape cannot be held, and ") + why +
                " reads a pool that is held; only the plain search with an explicit --target "
                "walks an addressed pool. Give --target k without " + why +
                ", or raise --max-memory if this machine really has the room");
        }
    }

    bilinear_rank::SearchBudget budget{node_limit, leaf_limit};
    std::vector<bilinear_rank::Matrix> products;
    const auto started = cli::Clock::now();

    bool found = false;
    if (target >= 0 && symmetry.kind != cli::SymmetryKind::None) {
        // The search needs a group that stabilises what it is searching from, and
        // `anchor` is not always the map: under `--anchor heuristic` it is the
        // heuristic's subspace, whose stabiliser is a different group.
        const std::vector<bilinear_rank::Automorphism> generators = bilinear_rank::stabiliser_of(
            field, anchor, bilinear_rank::requested_ambient_group(field, tensor.slices, symmetry));
        cli::result() << "  quotienting by " << generators.size() << " generators\n";
        // The quotient reads a held pool or an addressed one, whichever the shape
        // left available. It stopped needing the held one when its candidate list
        // became an index and its orbits a question rather than a table.
        found = fits ? bilinear_rank::expand_subspace_up_to_symmetry(
                           field, anchor, pool, generators,
                           static_cast<std::size_t>(target), budget, products)
                     : bilinear_rank::expand_subspace_up_to_symmetry(
                           field, anchor, addressed, generators,
                           static_cast<std::size_t>(target), budget, products);
    } else if (target >= 0 && !fits) {
        // Only a sweep still needs the pool held: it indexes down a recursion in
        // parallel and is not converted, and it is refused above.
        found = bilinear_rank::expand_subspace(field, anchor, addressed, 0,
                                              static_cast<std::size_t>(target), budget, products);
    } else if (target >= 0) {
        found = bilinear_rank::expand_subspace(field, anchor, pool, 0,
                                               static_cast<std::size_t>(target), budget, products);
    } else {
        found = bilinear_rank::fewest_products_by_sweep(field, anchor, pool, budget, products);
    }
    const double seconds = cli::elapsed_seconds(started);

    cli::result() << "  " << budget.nodes_visited << " nodes in " << seconds << " s\n";

    if (found) {
        cli::result() << "  FOUND: "
                      << bilinear_rank::require_bound_consistent(products.size(), bound) << "\n";
        bilinear_rank::Algorithm algorithm;
        if (!bilinear_rank::recovers_map(field, tensor.slices, products, algorithm)) {
            cli::note() << "FAILED: those products do not compute the map";
            // Was 1, which reads as "no such algorithm". The search did find
            // one and then its own check threw it out, so a component here is
            // wrong: that is Unverified, and it is worse news than a refusal.
            return cli::exit_status(cli::ExitCode::Unverified);
        }
        cli::result() << "  verified: they compute the map\n";
        return cli::exit_status(cli::ExitCode::Yes);
    }

    if (!budget.exhausted) {
        const bool leaf = budget.leaf_abandoned.load();
        cli::result() << "  GAVE UP: the " << (leaf ? "leaf" : "node")
                      << " limit was reached, so nothing is decided.\n"
                         "           Raise --"
                      << (leaf ? "leaf" : "node") << "-limit to search further.\n";
        // Was 2, the same code the two usage errors above return, so a script
        // could not tell a mistyped flag from a search that ran and gave up.
        // A spent budget decides nothing, which is exactly what 3 is for.
        return cli::exit_status(cli::ExitCode::Undecided);
    }
    if (target >= 0) {
        cli::result() << "  NO: there is no algorithm with " << target << " products"
                      << (anchor_on_heuristic ? " containing the heuristic's subspace" : "")
                      << ". The search was exhaustive.\n";
    } else {
        cli::result() << "  NO decomposition found in the searched range.\n";
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
        cli::note() << "decide-rank: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        // An unreadable file, or a run that would not fit the memory budget.
        // Reported as a line, not as a terminate. Was 1, which claimed no such
        // algorithm exists on the strength of a search that never started.
        cli::note() << "decide-rank: " << problem.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
