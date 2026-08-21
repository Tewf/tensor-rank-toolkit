/// Lower an upper bound: the exact search's tree, cut by what has been built
/// rather than by a target nobody has reached.
///
/// `decide-rank` answers a question and `minimise-rank` stops when no single map
/// pays. This one keeps going and reports the cheapest algorithm it holds, so a
/// spent budget is a weaker answer rather than no answer. It refutes nothing, and
/// every count it prints comes from a decomposition that was rebuilt and compared
/// against the map first.
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "arguments.h"
#include "candidate_pool.h"
#include "cost_first_search.h"
#include "exit_code.h"
#include "measures.h"
#include "minimise_rank.h"
#include "minimum_weight_basis.h"
#include "fewest_products.h"
#include "report.h"
#include "sms_file.h"
#include "tensor_file.h"

namespace {

void usage() {
    cli::note() << "usage: lower-the-bound <tensor-file> [--from basis|descent] [--nodes N]\n"
                   "                       [--width N] [--summand-rank r] [--whole-pool]\n"
                   "                       [--emit-operators <stem>] [--help]\n"
                   "\n"
                   "  --from basis|descent  the root and the first incumbent: the minimum-\n"
                   "                        weight basis of span(T), or the descent's own\n"
                   "                        answer through step 2. descent by default,\n"
                   "                        because a loose incumbent bounds nothing\n"
                   "  --nodes N             subspaces to expand, 20000 by default. Spending\n"
                   "                        it withdraws no answer: nothing here refutes\n"
                   "  --width N             children entered per node, cheapest first, 4 by\n"
                   "                        default. 0 enters every child, which is the\n"
                   "                        branch and bound rather than a beam\n"
                   "  --summand-rank r      the largest rank of a span element a move may\n"
                   "                        split, 3 by default. Raising it offers more\n"
                   "                        moves at (p^r - 1)p^(r-1)/(p-1) per element\n"
                   "  --whole-pool          offer every rank-one map of the shape instead,\n"
                   "                        which is |pool| minimum-weight bases a node\n"
                   "  --rounds N            restart from the answer up to N times, 8 by\n"
                   "                        default, stopping as soon as a round does not\n"
                   "                        improve. A round that exhausted its tree can\n"
                   "                        still improve from the subspace it ended on,\n"
                   "                        which is a different root\n"
                   "  --emit-operators <stem>  write <stem>_{L,R,P}.sms for the answer\n"
                   "  --help                print this and stop, as exit 2";
}

/// Does this spanning set still compute the map, and at the count claimed?
///
/// Asked in the tool and not only in the tests, under the rule
/// [`../../descent_search/README.md`](../../descent_search/README.md) states: a
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
    std::string operator_stem;

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
            limits.width = arguments.count();
        } else if (arguments.is("--summand-rank")) {
            limits.summand_rank = arguments.count();
        } else if (arguments.is("--whole-pool")) {
            limits.whole_pool = true;
        } else if (arguments.is("--rounds")) {
            rounds = arguments.count();
        } else if (arguments.is("--emit-operators")) {
            operator_stem = arguments.text();
        } else {
            arguments.refuse();
        }
    }
    if (arguments.reads_stdin()) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(arguments.filename());
    const bilinear_rank::Field field(tensor.characteristic);

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
    std::vector<bilinear_rank::Matrix> answer = start;
    std::size_t reached = linear_algebra::multiplication_count(field, start);
    for (std::size_t round = 0; round < rounds; ++round) {
        std::vector<bilinear_rank::Matrix> next =
            bilinear_rank::search_from_above(field, answer, pool, limits, &round_report);
        report.nodes += round_report.nodes;
        report.children += round_report.children;
        report.moves_offered += round_report.moves_offered;
        report.improvements += round_report.improvements;
        report.bounded += round_report.bounded;
        report.deepest = std::max(report.deepest, round_report.deepest);
        report.exhausted = round_report.exhausted;
        report.best = round_report.best;
        if (round_report.best >= reached) break;
        reached = round_report.best;
        answer = std::move(next);
    }

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
                << report.deepest << (report.exhausted ? ", tree exhausted" : ", budget spent");

    if (!operator_stem.empty()) {
        // The stem and the three suffixes are PLinOpt's interface, not a naming
        // choice here: `PMchecker stem_{L,R,P}.sms -q p` is how anything outside
        // this repository checks the answer.
        const std::string origin = "Encoding operator recovered by lower-the-bound, " +
                                   std::to_string(algorithm.product_count()) + " products, over GF(" +
                                   std::to_string(tensor.characteristic) + ").";
        linear_algebra::write_sms_file(operator_stem + "_L.sms", origin + " Left operand.",
                                       algorithm.left);
        linear_algebra::write_sms_file(operator_stem + "_R.sms", origin + " Right operand.",
                                       algorithm.right);
        linear_algebra::write_sms_file(operator_stem + "_P.sms",
                                       origin + " Combines the products into the outputs.",
                                       algorithm.decode);
        cli::note() << "wrote " << operator_stem << "_{L,R,P}.sms";
    }
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        cli::note() << "lower-the-bound: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& failure) {
        cli::note() << "lower-the-bound: " << failure.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
