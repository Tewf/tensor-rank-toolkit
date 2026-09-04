/// Tighten an upper bound: the exact search's tree, cut by what has been built
/// rather than by a target nobody has reached.
///
/// `decide-rank` answers a question and `minimise-rank` stops when no single map
/// pays. This one keeps going and reports the cheapest algorithm it holds, so a
/// spent budget is a weaker answer rather than no answer. It refutes nothing, and
/// every count it prints comes from a decomposition that was rebuilt and compared
/// against the map first.
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "card_failure_note.h"
#include "algorithm_recovery.h"
#include "arguments.h"
#include "candidate_pool.h"
#include "cost_first_search.h"
#include "exit_code.h"
#include "measures.h"
#include "memory_budget.h"
#include "minimise_rank.h"
#include "minimum_weight_basis.h"
#include "fewest_products.h"
#include "gf2_span_walk.h"
#include "parallel.h"
#include "report.h"
#include "requested_group.h"
#include "sms_file.h"
#include "tensor_file.h"

namespace {

void usage() {
    cli::note() << "usage: tighten-rank-bound <tensor-file> [--from basis|descent] [--nodes N]\n"
                   "                       [--width N] [--summand-rank r] [--whole-pool]\n"
                   "                       [--below k] [--orbit-moves] [--span-census]\n"
                   "                       [-s|--symmetry none|auto|matmul <n> <m> <k>]\n"
                   "                       [--general-span] [--emit-operators <stem>] [--help]\n"
                   "\n"
                   "  --from basis|descent  the root and the first incumbent: the minimum-\n"
                   "                        weight basis of span(T), or the descent's own\n"
                   "                        answer through step 2. descent by default,\n"
                   "                        because a loose incumbent bounds nothing\n"
                   "  --nodes N             subspaces to expand, 20000 by default. Spending\n"
                   "                        it withdraws no answer: nothing here refutes\n"
                   "  --width N|auto        children entered per node, cheapest first, 4 by\n"
                   "                        default. 0 enters every child, which is the\n"
                   "                        branch and bound rather than a beam. auto runs\n"
                   "                        at 4 and doubles once when the tree exhausted\n"
                   "                        with the budget unspent and the answer above the\n"
                   "                        floor: what-width-buys.md is why once\n"
                   "  --cost-drop s         the most one move may take off the cost, 0 by\n"
                   "                        default which leaves the bound as it was. A positive\n"
                   "                        s tightens it from dim+1 to the least value of\n"
                   "                        max(dim+t, cost-s*t), which at <3,4,5> is 38 rather\n"
                   "                        than 16. Measured at 1, not proved, so every child is\n"
                   "                        checked against it and a violation refuses the answer\n"
                   "  --summand-rank r      the largest rank of a span element a move may\n"
                   "                        split, 3 by default. Raising it offers more\n"
                   "                        moves at (p^r - 1)p^(r-1)/(p-1) per element\n"
                   "  --whole-pool          offer every rank-one map of the shape instead,\n"
                   "                        which is |pool| minimum-weight bases a node\n"
                   "  --below k             look only for k products or fewer, and stop the\n"
                   "                        moment one is reached. The incumbent is seeded at\n"
                   "                        k+1 instead of the start, so the bound cuts at\n"
                   "                        dimension k straight away and the tree is smaller.\n"
                   "                        Not reaching k refutes nothing: nothing here does\n"
                   "  --rounds N            restart from the answer up to N times, 8 by\n"
                   "                        default, stopping as soon as a round does not\n"
                   "                        improve. A round that exhausted its tree can\n"
                   "                        still improve from the subspace it ended on,\n"
                   "                        which is a different root\n"
                   "  --span-census         count how many of the nodes and children this run\n"
                   "                        visits are the same subspace reached twice, by a\n"
                   "                        different order of adjunctions. Off by default and\n"
                   "                        it changes nothing: a run with it on enters the same\n"
                   "                        tree and prints the same counts. What the number is\n"
                   "                        evidence about is whether isomorph rejection could\n"
                   "                        pay here at all\n"
                   "  --orbit-moves         offer one move per orbit at each node instead of\n"
                   "                        every move, under the group --symmetry names.\n"
                   "                        Off by default: every count published for this\n"
                   "                        search was taken without it, and no group is\n"
                   "                        available for most fixtures here\n"
                << cli::symmetry_usage()
                << "  --general-span        walk every span by the general field path, even\n"
                   "                        over GF(2) where the bit-packed one applies.\n"
                   "                        Same tree, same nodes, same answer, and slower:\n"
                   "                        it is here so the two can be timed on one\n"
                   "                        question rather than on two\n"
                   "  --emit-operators <stem>  write <stem>_{L,R,P}.sms for the answer\n"
                   "  --threads N           N workers, 0 for every core, 1 by default. The\n"
                   "                        children of one node are prepared in parallel and\n"
                   "                        entered in the same order at any count, so every\n"
                   "                        number this prints is what one worker printed\n"
                   "  --max-memory N        bytes one bulk allocation may take, 2G by\n"
                   "                        default. --summand-rank r asks for p^r vectors,\n"
                   "                        and this is what refuses an r the machine cannot\n"
                   "                        hold\n"
                   "  --help                print this and stop, as exit 2";
}

/// The share of a population that was a subspace already reached, as a
/// percentage to one decimal.
///
/// `(recorded - distinct) / recorded` and not `distinct / recorded`: the
/// question is what a parent test would remove, which is every arrival after the
/// first and not every subspace with a duplicate.
double repeated_share(const bilinear_rank::SpanTally& tally) {
    if (tally.recorded() == 0) return 0.0;
    const double repeats = static_cast<double>(tally.recorded() - tally.distinct());
    return std::round(1000.0 * repeats / static_cast<double>(tally.recorded())) / 10.0;
}

/// Does this spanning set still compute the map, and at the count claimed?
///
/// Asked in the tool and not only in the tests, under the rule
/// [`../../greedy_heuristic/`](../../greedy_heuristic/) states: a
/// search that quietly loses a slice reports excellent numbers.
bool verified(const bilinear_rank::Field& field, const std::vector<bilinear_rank::Matrix>& target,
              const std::vector<bilinear_rank::Matrix>& answer,
              bilinear_rank::Algorithm& algorithm) {
    const std::vector<bilinear_rank::Matrix> products =
        bilinear_rank::rank_one_candidates(field, answer);
    if (!bilinear_rank::recovers_map(field, target, products, algorithm)) return false;
    return algorithm.product_count() == linear_algebra::multiplication_count(field, answer);
}

int run(int argc, char** argv) {
    bilinear_rank::IncumbentLimits limits;
    bool from_descent = true;
    std::size_t rounds = 8;
    bool counting_spans = false;
    // `--width auto`: widen once, and only on the evidence that widening is the
    // thing that was missing. Measured 2026-08-22, see `what-width-buys.md`.
    bool widen = false;
    std::string operator_stem;
    cli::Symmetry symmetry;

    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--from")) {
            const std::string where = arguments.text();
            if (where == "basis") {
                from_descent = false;
            } else if (where == "descent") {
                from_descent = true;
            } else {
                arguments.refuse();
            }
        } else if (arguments.is("--nodes")) {
            limits.node_limit = arguments.count();
        } else if (arguments.is("--width")) {
            const std::string asked = arguments.text();
            if (asked == "auto") {
                widen = true;
            } else {
                limits.width = cli::parse_count("--width", asked);
            }
        } else if (arguments.is("--cost-drop")) {
            limits.cost_drop_bound = arguments.count();
        } else if (arguments.is("--summand-rank")) {
            limits.summand_rank = arguments.count();
        } else if (arguments.is("--whole-pool")) {
            limits.whole_pool = true;
        } else if (arguments.is("--below")) {
            limits.below = arguments.count();
        } else if (arguments.is("--orbit-moves")) {
            limits.quotient_moves = true;
        } else if (arguments.is("--span-census")) {
            counting_spans = true;
        } else if (arguments.is("--symmetry", "-s")) {
            symmetry = arguments.parsed_by(cli::parse_symmetry);
        } else if (arguments.is("--rounds")) {
            rounds = arguments.count();
        } else if (arguments.is("--threads")) {
            run_limits::set_worker_count(arguments.count());
        } else if (arguments.is("--max-memory")) {
            // Every `require_room` this command reaches was pinned at the
            // compiled 2 GiB, while the refusal it prints ends "Raise it with
            // --max-memory if the machine has the room": naming a flag this
            // command did not have. It has it now.
            run_limits::set_memory_budget(arguments.memory_size());
        } else if (arguments.is("--general-span")) {
            // Step 1's span walk in field elements rather than in bits.
            // `gf2_span_walk_applies` is asked once per call, inside
            // `minimum_weight_basis`, and has to see this before the first one.
            bilinear_rank::set_gf2_span_walk_offered(false);
        } else if (arguments.is("--emit-operators")) {
            operator_stem = arguments.text();
        } else {
            arguments.refuse();
        }
    }
    if (arguments.no_file_named()) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    const formats::Tensor tensor = formats::read_tensor_file(arguments.filename());
    const bilinear_rank::Field field(tensor.characteristic);

    // The **ambient** group, which each node narrows to its own stabiliser.
    //
    // Built before anything else runs, because `--symmetry auto` **refuses**
    // rather than building 9.99872e13 automorphisms for a 5x5 map over GF(2),
    // and a refusal belongs before a run and not after the descent has spent a
    // minute earning the root it will never search from.
    std::vector<bilinear_rank::Automorphism> ambient;
    if (limits.quotient_moves) {
        if (symmetry.kind == cli::SymmetryKind::None) {
            cli::note() << "--orbit-moves without --symmetry has no group to quotient by, so "
                           "every move is offered and the run is the unquotiented one";
        }
        ambient = bilinear_rank::requested_ambient_group(field, tensor.slices, symmetry);
        cli::note() << "ambient group: " << ambient.size() << " automorphisms";
    } else if (symmetry.kind != cli::SymmetryKind::None) {
        cli::note() << "--symmetry was given without --orbit-moves, so nothing reads it";
    }

    const std::vector<bilinear_rank::Matrix> start =
        from_descent ? bilinear_rank::descend_from_own_basis(field, tensor.slices)
                     : bilinear_rank::minimum_weight_basis(field, tensor.slices);
    cli::result() << "GF(" << tensor.characteristic << "), start: "
                  << linear_algebra::multiplication_count(field, start) << " products over "
                  << start.size() << " dimensions\n";

    // Built only where it is asked for. The generated moves never touch it, and
    // at 7x7 over GF(2) it is 16 129 matrices that would otherwise be formed to
    // be ignored.
    std::vector<bilinear_rank::Matrix> pool;
    if (limits.whole_pool) {
        pool = bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
        cli::note() << "pool: " << pool.size() << " rank-one maps";
    }

    // Restarting from the answer is not the same search again. A round that
    // exhausted its tree exhausted the tree *above its own root*, and the answer
    // is a different subspace: the next round starts higher up and under a
    // tighter incumbent, so it cuts sooner and reaches further.
    bilinear_rank::IncumbentReport report;
    bilinear_rank::IncumbentReport round_report;
    // One census for the whole run and not one a round, because a subspace two
    // rounds both reach is reached twice: `--rounds` restarts from the answer,
    // which is a node the round before it walked through.
    bilinear_rank::SpanCensus census;
    std::vector<bilinear_rank::Matrix> answer = start;
    std::size_t reached = linear_algebra::multiplication_count(field, start);
    const auto run_rounds = [&] {
        for (std::size_t round = 0; round < rounds; ++round) {
            std::vector<bilinear_rank::Matrix> next =
                bilinear_rank::search_from_above(field, answer, pool, limits, &round_report,
                                                 ambient, counting_spans ? &census : nullptr);
            report.nodes += round_report.nodes;
            report.children += round_report.children;
            report.moves_offered += round_report.moves_offered;
            report.moves_entered += round_report.moves_entered;
            report.smallest_stabiliser =
                report.largest_stabiliser == 0
                    ? round_report.smallest_stabiliser
                    : std::min(report.smallest_stabiliser, round_report.smallest_stabiliser);
            report.largest_stabiliser =
                std::max(report.largest_stabiliser, round_report.largest_stabiliser);
            report.improvements += round_report.improvements;
        report.largest_drop = std::max(report.largest_drop, round_report.largest_drop);
        report.drops_exceeded += round_report.drops_exceeded;
            report.bounded += round_report.bounded;
            report.deepest = std::max(report.deepest, round_report.deepest);
            report.tree_fully_walked = round_report.tree_fully_walked;
            report.best = round_report.best;
            report.reached_below = round_report.reached_below;
            if (round_report.best >= reached) break;
            reached = round_report.best;
            answer = std::move(next);
            // What `--below` asked for is held, so another round is another search
            // for something nobody asked about.
            if (report.reached_below) break;
        }
    };

    run_rounds();

    // **Widening, once, and only on the evidence that width was what was
    // missing.** Three things have to hold together, and each rules out a
    // different reason a run stopped where it did: the tree was *exhausted*, so
    // more budget cannot help and only a wider beam can; the budget is mostly
    // unspent, so there is room to pay for one; and the answer is still above
    // the floor, so there is something left to reach.
    //
    // Measured at widths 1, 2, 4, 8 and 16 over ten fixtures
    // ([`../what-width-buys.md`](../what-width-buys.md)): width changes the
    // answer on exactly two, and on both of those it is 8 that changes it. No
    // fixture measured wants 16, which is why this doubles once and stops rather
    // than climbing. It is not free: `gf32_multiplication` is 368 nodes and
    // seconds at 4 against 1 873 nodes and 466 s at 8, which is why it is a flag
    // and not the default.
    if (widen && report.tree_fully_walked && !report.reached_below &&
        report.nodes * 2 < limits.node_limit &&
        report.best > bilinear_rank::flattening_floor(field, tensor.slices)) {
        limits.width *= 2;
        cli::note() << "widening: the tree exhausted at " << report.nodes
                    << " of " << limits.node_limit << " nodes with " << report.best
                    << " above the floor, so entering " << limits.width
                    << " children a node instead";
        run_rounds();
    }

    // The bound checked its own assumption while it ran. If the assumption was
    // false on this map then a branch was cut that could have held the answer,
    // and the number below would be an upper bound arrived at unsoundly. Say so
    // and refuse rather than print it: a wrong bound that looks like a result is
    // the one outcome worse than no result.
    if (limits.cost_drop_bound != 0 && report.drops_exceeded != 0) {
        cli::note() << "--cost-drop " << limits.cost_drop_bound << " was violated "
                    << report.drops_exceeded << " times, the largest drop seen being "
                    << report.largest_drop
                    << ". The bound cut branches it had no right to cut, so no count is"
                       " reported. Re-run with --cost-drop " << report.largest_drop
                    << " or without the flag";
        return cli::exit_status(cli::ExitCode::Unverified);
    }

    bilinear_rank::note_if_the_card_failed();

    bilinear_rank::Algorithm algorithm;
    if (!verified(field, tensor.slices, answer, algorithm)) {
        cli::note() << "the answer did not rebuild the map, so its count is not reported";
        return cli::exit_status(cli::ExitCode::Unverified);
    }

    const std::size_t floor = bilinear_rank::flattening_floor(field, tensor.slices);
    cli::result() << "best: "
                  << bilinear_rank::require_bound_consistent(algorithm.product_count(), floor)
                  << ", verified\n";
    cli::note() << report.nodes << " nodes, " << report.children << " children costed, "
                << report.moves_offered << " moves offered, " << report.improvements
                << " improvements, " << report.bounded << " branches bounded, depth "
                << report.deepest << ", largest single-move drop " << report.largest_drop
                << (report.tree_fully_walked ? ", tree exhausted" : ", budget spent");

    // What the tree repeated, which is the whole evidence about whether an
    // isomorph rejection scheme could pay on this search. Printed as a rate as
    // well as a pair, because "3 of 22" and "3%" carry the decision and the
    // ratio alone does not say how big the population was.
    if (counting_spans) {
        cli::note() << "span census: " << census.entered.recorded() << " nodes entered, "
                    << census.entered.distinct() << " distinct spans, "
                    << repeated_share(census.entered) << "% repeats, most-entered span reached "
                    << census.entered.most_repeated() << " times";
        cli::note() << "span census: " << census.expanded.recorded() << " nodes expanded, "
                    << census.expanded.distinct() << " distinct spans, "
                    << repeated_share(census.expanded) << "% repeats, most-expanded span reached "
                    << census.expanded.most_repeated() << " times";
        cli::note() << "span census: " << census.children.recorded() << " children costed, "
                    << census.children.distinct() << " distinct spans, "
                    << repeated_share(census.children) << "% repeats, most-costed span reached "
                    << census.children.most_repeated() << " times";
    }

    // Said in words rather than left to be inferred from the count, and said
    // both ways round: a run that did not reach `k` has proved nothing about
    // `k`, and the branches it cut were cut by `k` itself.
    // What the quotient did, printed whether it did anything or not: a stabiliser
    // of one quotients nothing, and a run should say so rather than leave a
    // reader to infer it from an unchanged count.
    if (limits.quotient_moves) {
        cli::note() << "orbit-moves: " << report.moves_entered << " of " << report.moves_offered
                    << " moves entered, stabiliser " << report.smallest_stabiliser << " to "
                    << report.largest_stabiliser << " over the nodes";
    }

    if (limits.below != 0) {
        if (report.reached_below) {
            cli::note() << "--below " << limits.below << ": reached, at "
                        << algorithm.product_count() << " products";
        } else {
            cli::note() << "--below " << limits.below << ": not reached"
                        << (report.tree_fully_walked ? ", the tree above the root ran out"
                                             : ", the node budget ran out")
                        << ". That is not a lower bound: this search only ever finds, and "
                           "the branches it cut were cut by " << limits.below
                        << " and not by anything built";
        }
    }

    if (!operator_stem.empty()) {
        // The stem and the three suffixes are PLinOpt's interface, not a naming
        // choice here: `PMchecker stem_{L,R,P}.sms -q p` is how anything outside
        // this repository checks the answer.
        const std::string origin = "Encoding operator recovered by tighten-rank-bound, " +
                                   std::to_string(algorithm.product_count()) + " products, over GF(" +
                                   std::to_string(tensor.characteristic) + ").";
        formats::write_sms_file(operator_stem + "_L.sms", origin + " Left operand.",
                                       algorithm.left);
        formats::write_sms_file(operator_stem + "_R.sms", origin + " Right operand.",
                                       algorithm.right);
        formats::write_sms_file(operator_stem + "_P.sms",
                                       origin + " Combines the products into the outputs.",
                                       algorithm.decode);
        cli::note() << "wrote " << operator_stem << "_{L,R,P}.sms";
    }
    // `Undecided` and never `No`: a `--below` run that ran out has exhausted a
    // budget and refuted nothing, which is the distinction `infrastructure/cli/exit_code.h`
    // exists to keep. The answer it still holds was printed and verified above.
    if (limits.below != 0 && !report.reached_below) {
        return cli::exit_status(cli::ExitCode::Undecided);
    }
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        cli::note() << "tighten-rank-bound: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& failure) {
        cli::note() << "tighten-rank-bound: " << failure.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
