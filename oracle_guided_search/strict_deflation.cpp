#include "strict_deflation.h"

#include "group_construction.h"

#include <mutex>
#include <stdexcept>

#include "binary_encoding.h"
#include "candidate_pool.h"
#include "exit_code.h"
#include "measures.h"
#include "orbit_cubes.h"
#include "parallel.h"
#include "pool_orbits.h"
#include "timing.h"
#include "tree_refutation.h"

namespace bilinear_rank {

namespace {

/// The candidates, as the matrices the cubes would have pinned.
///
/// Taken from the closed-form orbits so that the two refuters are asked about the
/// same candidates in the same order. A comparison of two methods on two different
/// candidate sets would measure the sets.
std::vector<Matrix> candidates_of(const Field& field, const std::vector<std::size_t>& shape) {
    if (shape.size() != 3) return {};
    return matrix_multiplication_orbit_representatives(field, shape[0], shape[1], shape[2]);
}

/// A named shape must be this tensor's shape, on both routes.
///
/// Neither route reaches `requested_ambient_group`, which is where the same guard
/// sits for the three commands that do, so it is asked for here. Without it the
/// tree route built 4x4 representatives for a tensor of 4x6 slices and died in
/// the allocator: `free(): invalid pointer`, exit 134, which is not in
/// `infrastructure/cli/exit_code.h`'s vocabulary at all.
void require_shape_or_nothing(const formats::Tensor& tensor,
                              const std::vector<std::size_t>& shape) {
    if (shape.size() != 3) return;
    const Field field(tensor.characteristic);
    require_matmul_shape(tensor.slices, shape[0], shape[1], shape[2]);
    (void)field;
}

void verify_or_throw(const Field& field, const formats::Tensor& tensor, StrictStep& step) {
    std::vector<Matrix> kept;
    for (const Matrix& term : step.decomposition) {
        if (linear_algebra::nonzero_count(field, term) > 0) kept.push_back(term);
    }
    step.decomposition = std::move(kept);
    if (!recovers_map(field, tensor.slices, step.decomposition, step.algorithm)) {
        throw cli::CheckFailed("a strict step accepted a decomposition that does not compute "
                               "the map");
    }
}

/// The solver route. One `decide_rank` call over every cube, with the large budget,
/// waiting for unsatisfiable on each.
StrictStep by_solver(const formats::Tensor& tensor, std::size_t products,
                     const StrictSettings& settings) {
    require_shape_or_nothing(tensor, settings.matmul_shape);
    StrictStep step;
    step.products = products;

    satisfiability::SolveOptions approach = settings.approach;
    approach.timeout_seconds = settings.candidate_seconds;
    approach.probe_seconds = 0;
    if (settings.matmul_shape.size() == 3 && tensor.characteristic == 2) {
        const satisfiability::BinaryEncoding numbering =
            satisfiability::encode_binary_rank_at_most(tensor, products);
        const Field field(tensor.characteristic);
        approach.cubes = orbit_cubes(field, tensor.slices, settings.matmul_shape[0],
                                     settings.matmul_shape[1], settings.matmul_shape[2],
                                     numbering.left, numbering.right);
    }

    satisfiability::CubeReport report;
    const satisfiability::Answer answer =
        satisfiability::decide_rank(tensor, products, approach, &report);
    for (std::size_t index = 0; index < report.verdict.size(); ++index) {
        step.verdicts.push_back({index, report.verdict[index], report.seconds[index], 0});
    }

    step.solver_name = answer.solver_name;
    step.accepted = answer.verdict == satisfiability::Verdict::Yes;
    step.refuted = answer.verdict == satisfiability::Verdict::No;
    if (step.accepted) {
        step.decomposition = answer.decomposition;
        const Field field(tensor.characteristic);
        verify_or_throw(field, tensor, step);
    }
    return step;
}

/// The tree route, candidate by candidate.
StrictStep by_tree(const formats::Tensor& tensor, std::size_t products,
                   const StrictSettings& settings) {
    const Field field(tensor.characteristic);
    require_shape_or_nothing(tensor, settings.matmul_shape);
    StrictStep step;
    step.products = products;

    const std::vector<Matrix> candidates = candidates_of(field, settings.matmul_shape);
    if (candidates.empty()) {
        throw std::invalid_argument(
            "the tree refuter needs the closed-form orbits, so name the shape with "
            "--symmetry matmul <n> <m> <k>");
    }
    // Considered for `RankOnePool` and deliberately left materialised.
    // `tree_verdict` hands this to a refutation tree that indexes it down the
    // recursion, and every candidate is asked against the same pool, in parallel
    // when the settings allow. An addressed pool would rebuild each map once per
    // node per candidate rather than once per run.
    const std::vector<Matrix> pool =
        all_rank_one_maps(field, tensor.rows(), tensor.columns());
    const std::vector<Automorphism> ambient = ambient_or_empty(field, settings.matmul_shape);

    std::mutex guard;
    std::vector<CandidateVerdict> verdicts(candidates.size());
    std::vector<std::vector<Matrix>> found(candidates.size());
    // Which candidates were actually asked, recorded rather than inferred from a
    // zero time: a sequential run stops at the first acceptance and leaves the rest
    // untouched, and "untouched" must not be reported as a verdict.
    std::vector<char> asked(candidates.size(), 0);
    const auto ask = [&](std::size_t index) {
        std::vector<Matrix> mine;
        // One fan-out, not two: this loop already has the cores when
        // `parallel_candidates` is on, and the tree inside would otherwise start a
        // second set of workers per candidate.
        CandidateVerdict verdict =
            tree_verdict(field, tensor, products, candidates[index], pool, ambient,
                         settings.node_limit, mine, !settings.parallel_candidates);
        verdict.candidate = index;
        const std::lock_guard<std::mutex> held(guard);
        verdicts[index] = verdict;
        found[index] = std::move(mine);
        asked[index] = 1;
    };

    if (settings.parallel_candidates) {
        run_limits::parallel_for(candidates.size(), ask);
    } else {
        // Sequential stops at the first acceptance, which the parallel form cannot
        // and does not: it prices every candidate, which is what a table wants.
        for (std::size_t index = 0; index < candidates.size(); ++index) {
            ask(index);
            if (verdicts[index].verdict == satisfiability::Verdict::Yes) break;
        }
    }

    // Refuted only if every candidate was asked and every one refused. A sequential
    // run that stopped early has candidates it never asked, and those leave the
    // claim unproved rather than supporting it.
    step.refuted = true;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (asked[index] == 0) {
            step.refuted = false;
            continue;
        }
        step.verdicts.push_back(verdicts[index]);
        if (verdicts[index].verdict == satisfiability::Verdict::Yes && !step.accepted) {
            step.accepted = true;
            step.decomposition = found[index];
        }
        if (verdicts[index].verdict != satisfiability::Verdict::No) step.refuted = false;
    }
    if (step.accepted) {
        step.refuted = false;
        verify_or_throw(field, tensor, step);
    }
    return step;
}

}  // namespace

StrictStep strict_step(const formats::Tensor& tensor, std::size_t products,
                       const StrictSettings& settings) {
    const cli::Clock::time_point started = cli::Clock::now();
    StrictStep step = settings.refuter == Refuter::QuotientedTree
                          ? by_tree(tensor, products, settings)
                          : by_solver(tensor, products, settings);
    step.seconds = cli::elapsed_seconds(started);
    return step;
}

}  // namespace bilinear_rank
