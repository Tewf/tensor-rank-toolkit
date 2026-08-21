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
#include "kronecker_structure.h"
#include "card_failure_note.h"
#include "dense_matrix_file.h"
#include "device.h"
#include "exhaustive_search.h"
#include "exit_code.h"
#include "fewest_products.h"
#include "gf2_leaf.h"
#include "group_construction.h"
#include "isomorph_rejection.h"
#include "memory_budget.h"
#include "minimise_rank.h"
#include "orbit_search.h"
#include "parallel.h"
#include "plan_file.h"
#include "rank_one_basis.h"
#include "report.h"
#include "requested_group.h"
#include "search_plan.h"
#include "size_argument.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "timing.h"
#include "tunables.h"

namespace {

void usage() {
    cli::note() << "usage: decide-rank <tensor-file> [--target k] [--anchor map|heuristic]\n"
                   "                   [--node-limit N] [--leaf-limit N]\n"
                   "                   [--max-memory 2G] [--general-leaf]\n"
                   "                   [--leaf-route auto|scan|walk] [--help]\n"
                   "                   [--orbit-test full|generators]\n"
                   "                   [--device cpu|gpu|auto]\n"
                   "                   [--plan-out FILE] [--plan-in FILE]\n"
                   "                   [--threads N]   N workers, 0 for every core, 1 by default\n"
                   "                   [-s|--symmetry none|auto|matmul <n> <m> <k>]\n"
                   "\n"
                   "  --device D          which processor answers a leaf. auto, the default, takes\n"
                   "                      the card where one is compiled in, present, has a kernel\n"
                   "                      for the shape, and the leaf is over device_launch_floor\n"
                   "                      elements. gpu asks for it and lifts that floor; cpu takes\n"
                   "                      it off the table. gpu is a request and not an instruction:\n"
                   "                      where the card cannot answer, the host does and says why\n"
                   "  --plan-out FILE     write the seven choices this run made, and carry on\n"
                   "  --plan-in FILE      make those choices instead of deciding them here, so a\n"
                   "                      run elsewhere reproduces this one. A flag given beside it\n"
                   "                      still wins: flag, then file, then rule. The numbers a run\n"
                   "                      is bounded by travel in tunables.conf, not in a plan\n"
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
                   "  --leaf-route R      force a leaf onto one route: scan the pool, or walk the\n"
                   "                      subspace. Default auto, which takes the cheaper by size.\n"
                   "                      For timing one against the other on one question; walk is\n"
                   "                      ignored where the subspace is too large to enumerate.\n"
                   "  --orbit-test R      how -s rejects a repeated branch. full, the default, keeps\n"
                   "                      the least member of each orbit and walks the orbit to find\n"
                   "                      it; generators tests only the images under the surviving\n"
                   "                      generators, which is cheaper per candidate and leaves\n"
                   "                      duplicate branches standing. Same verdict either way, and\n"
                   "                      the node counts say what the duplication costs.\n"
                   "  --help              print this and stop, as exit 2";
}

/// The tool proper. main only turns a thrown refusal into a line.
int run(int argc, char** argv) {
    long long target = -1;
    // The file first, so that `--node-limit` below can overwrite it: a flag that
    // was given always wins over tunables.conf, and one that was not leaves the
    // file's number standing.
    // The device ranking and the launch floor are in force before the plan is
    // chosen, because the plan asks `run_limits::chosen_device` what they say.
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
    // Collected rather than applied as they are read: four of these are what the
    // plan is made of, and a setting applied during the walk would be a decision
    // made before the tensor that decides it has been opened.
    bilinear_rank::PlanRequest request;
    bilinear_rank::PlanFlagsGiven given;
    std::string plan_out;
    std::string plan_in;

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
            const std::string from = arguments.text();
            if (from != "map" && from != "heuristic") {
                throw cli::ArgumentError("--anchor expects map or heuristic, not '" + from + "'");
            }
            request.anchor = from == "heuristic" ? bilinear_rank::Anchor::Heuristic
                                                 : bilinear_rank::Anchor::Map;
            given.anchor = true;
        } else if (arguments.is("--symmetry", "-s")) {
            request.quotient = arguments.parsed_by(cli::parse_symmetry);
            given.quotient = true;
        } else if (arguments.is("--threads")) {
            request.threads = arguments.count();
            given.threads = true;
        } else if (arguments.is("--max-memory")) {
            bilinear_rank::set_memory_budget(arguments.memory_size());
        } else if (arguments.is("--node-limit")) {
            node_limit = arguments.count();
        } else if (arguments.is("--leaf-limit")) {
            leaf_limit = arguments.count();
        } else if (arguments.is("--general-leaf")) {
            // Applied here and not through the plan: it chooses which
            // implementation of the leaf test answers, where the plan chooses
            // which route and which processor. `gf2_leaf_applies` is asked
            // below and has to see it.
            bilinear_rank::set_gf2_leaf_offered(false);
        } else if (arguments.is("--device")) {
            const std::string wanted = arguments.text();
            if (wanted == "cpu") request.device = bilinear_rank::DeviceRequest::Cpu;
            else if (wanted == "gpu") request.device = bilinear_rank::DeviceRequest::Gpu;
            else if (wanted != "auto") {
                throw cli::ArgumentError("--device expects cpu, gpu or auto, not '" + wanted + "'");
            }
            given.device = wanted != "auto";
        } else if (arguments.is("--plan-out")) {
            plan_out = arguments.text();
        } else if (arguments.is("--plan-in")) {
            plan_in = arguments.text();
        } else if (arguments.is("--leaf-route")) {
            const std::string route = arguments.text();
            if (route == "scan") request.leaf_route = bilinear_rank::LeafRoute::Scan;
            else if (route == "walk") request.leaf_route = bilinear_rank::LeafRoute::Walk;
            else if (route != "auto") {
                throw cli::ArgumentError("--leaf-route expects auto, scan or walk, not '" + route +
                                         "'");
            }
            given.leaf_route = route != "auto";
        } else if (arguments.is("--orbit-test")) {
            const std::string rule = arguments.text();
            if (rule == "generators") {
                request.orbit_test = bilinear_rank::OrbitTest::Generators;
                given.orbit_test = true;
            } else if (rule == "full") {
                given.orbit_test = true;
            } else {
                // Named and quoted rather than `arguments.refuse()`, which is
                // what the branch above does and which reports a bad *value* as
                // an unrecognised *flag*: `--leaf-route bogus` leaves as
                // "unrecognised option: --leaf-route", naming neither the word
                // that was wrong nor the two that would have been right. That is
                // the fault `arguments.h` exists to remove and there is no reason
                // to copy it into a new flag.
                throw cli::ArgumentError("--orbit-test expects full or generators, not '" +
                                         rule + "'");
            }
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

    // Two slices is a different problem, and a polynomial one. A pencil `A + xB`
    // has a complete invariant — Kronecker's minimal indices and elementary
    // divisors — so where that invariant settles the rank there is nothing to
    // search for and no pool to build. Håstad's NP-completeness is about tensors
    // whose third dimension grows; it does not reach this case, and declining to
    // use the closed form here would be walking an exponential tree past an
    // answer already in hand.
    //
    // **Only when `exact`.** `pencil_rank_of` also returns a proved *bound* when
    // it cannot settle the rank, and that bound is not worth taking: on a random
    // 8x8 pencil over GF(2) it says 9 where `flattening_floor` above already says
    // 11, and `projection_lower_bound.h` records the same ordering. So an
    // inexact pencil falls through to the search with nothing borrowed from it.
    //
    // **Where it fires is a question about the field, not the shape**, and that is
    // what makes it worth having. `[sumi2009, Thm. 3.3]` settles the regular case
    // whenever `Card(K) >= deg p_1(A)`, so over a large enough field almost every
    // pencil is exact and over GF(2) many are not. Measured 2026-08-21: a random
    // 7x7 pencil over GF(11) is settled here in 51 microseconds, and the pool the
    // tree would otherwise need for that shape is `((11^7-1)/10)^2`, about
    // 3.8e12 rank-one maps, which `require_room` refuses long before any search.
    // Over GF(2) the same construction is usually inexact and falls through.
    if (tensor.slices.size() <= 2 && !tensor.slices.empty()) {
        const pencil_rank::ModularField pencil_field(tensor.characteristic);
        const pencil_rank::PencilRank settled =
            pencil_rank::pencil_rank_of(pencil_field, tensor.slices);
        if (settled.exact) {
            cli::result() << "  pencil: two slices, so the Kronecker canonical form settles it"
                          << " in polynomial time and no pool is built\n";
            if (target < 0) {
                cli::result() << "  rank: " << settled.proved << " (exact, over GF("
                              << tensor.characteristic << "))\n";
                return cli::exit_status(cli::ExitCode::Yes);
            }
            const bool reachable = static_cast<std::size_t>(target) >= settled.proved;
            cli::result() << (reachable ? "  YES: " : "  NO: there is no algorithm with ")
                          << target << " products"
                          << (reachable ? " suffice, the rank being " : ", the rank being ")
                          << settled.proved << ".\n";
            return cli::exit_status(reachable ? cli::ExitCode::Yes : cli::ExitCode::No);
        }
    }

    // The pool is addressed rather than materialised where it would not fit. The
    // recursion carries an index down and resumes from it, so an addressed pool
    // rebuilds a map once per node that reaches it rather than once per run. That
    // is a real cost and it is why the materialised pool stays the default. It is
    // also plainly the right trade when the alternative is refusing to start: at
    // `⟨4,4,4⟩` the pool is 4.3e9 maps and 8.2 TiB, and a slower search beats no
    // search. This is the odometer of `[yang2025]`, whose whole difference from
    // `[bdez2012]` Algorithm 1 is that it never holds the pool. The rule itself
    // is in `search_plan/`, with the other six.
    const bilinear_rank::RankOnePool addressed(field, tensor.rows(), tensor.columns());
    cli::result() << "  pool: " << addressed.size() << " rank-one maps of shape "
                  << tensor.rows() << "x" << tensor.columns() << "\n";

    // Which leaf test answered, printed for the same reason the pool line is:
    // a timing whose route is not on the line beside it is a timing of an
    // unknown thing, and `--general-leaf` is here precisely to move this.
    const bool binary_leaf = bilinear_rank::gf2_leaf_applies(field, tensor.columns());
    cli::result() << "  leaf: "
                  << (binary_leaf ? "GF(2), one bit per entry" : "general field path") << "\n";

    // Everything the seven rules read, in one struct, so that what a run decided
    // can be printed, written down and replayed rather than reconstructed by
    // whoever reads the source next.
    bilinear_rank::Machine machine;
    machine.characteristic = static_cast<std::size_t>(tensor.characteristic);
    machine.rows = tensor.rows();
    machine.columns = tensor.columns();
    machine.pool_size = addressed.size();
    machine.target = target >= 0 ? static_cast<std::size_t>(target) : 0;
    machine.memory_budget = bilinear_rank::memory_budget();
    machine.binary_leaf = binary_leaf;

    // `--threads 0` means every core, and the plan reports the count that was
    // resolved rather than the nought that asked for it.
    bilinear_rank::set_worker_count(request.threads);
    request.threads = bilinear_rank::worker_count();

    bilinear_rank::SearchPlan plan =
        plan_in.empty()
            ? bilinear_rank::chosen_plan(machine, request)
            : bilinear_rank::plan_with_flags(bilinear_rank::read_plan_file(plan_in), request,
                                             given);
    // A plan naming the card is a fact about the machine it was taken on, so a
    // replayed one is asked again here. A card that is missing or has no kernel
    // for this shape must never produce a wrong answer, and declining is how.
    if (plan.device == run_limits::Device::Gpu) {
        if (const char* refused = bilinear_rank::card_refusal(machine)) {
            plan.device = run_limits::Device::Cpu;
            plan.device_reason = std::string(refused) + ", so the plan's gpu was declined";
        }
    }

    cli::result() << "  plan:\n";
    for (const std::string& line : bilinear_rank::plan_lines(plan)) {
        cli::result() << "    " << line << "\n";
    }
    if (!plan_out.empty()) bilinear_rank::write_plan_file(plan_out, plan);

    // Applied here, all seven at once, which is the point of the struct: a
    // setting written where its flag is read is a decision nothing can report.
    bilinear_rank::set_worker_count(plan.threads);
    bilinear_rank::set_leaf_route(plan.leaf_route);
    bilinear_rank::set_orbit_test(plan.orbit_test);
    {
        // The ranking is how the device decision reaches a leaf: `Gf2Leaf` asks
        // `chosen_device` with its own element count, so the floor still keeps a
        // small leaf here even where the card is on the table. `--device gpu`
        // lifts the floor, which is the only thing left for it to override.
        std::string unrecognised;
        run_limits::set_device_order(plan.device == run_limits::Device::Gpu
                                         ? std::vector<std::string>{"gpu", "cpu"}
                                         : std::vector<std::string>{"cpu"},
                                     unrecognised);
        if (request.device == bilinear_rank::DeviceRequest::Gpu) run_limits::set_launch_floor(0);
    }

    std::vector<bilinear_rank::Matrix> anchor = tensor.slices;
    const bool anchor_on_heuristic = plan.anchor == bilinear_rank::Anchor::Heuristic;
    if (anchor_on_heuristic) {
        anchor = bilinear_rank::descend_from_own_basis(field, tensor.slices);
        cli::result() << "anchored on the heuristic: " << anchor.size() << " slices, "
                      << linear_algebra::multiplication_count(field, anchor)
                      << " multiplications\n";
    }

    const bool fits = plan.pool == bilinear_rank::Pool::Materialised;
    const cli::Symmetry symmetry = plan.quotient;
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
        // Which rejection rule, on the same line as the group, for the reason
        // the pool and leaf lines say which route they took: a node count whose
        // rule is not beside it is a node count of an unknown tree.
        cli::result() << "  quotienting by " << generators.size() << " generators, "
                      << (bilinear_rank::orbit_test() == bilinear_rank::OrbitTest::Generators
                              ? "rejecting by generators only"
                              : "rejecting exactly")
                      << "\n";
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
    bilinear_rank::note_if_the_card_failed();

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
