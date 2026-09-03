#include "tree_refutation.h"

#include <stdexcept>

#include "exhaustive_search.h"
#include "group_construction.h"
#include "orbit_search.h"
#include "timing.h"

namespace bilinear_rank {

std::vector<Automorphism> ambient_or_empty(const Field& field,
                                           const std::vector<std::size_t>& shape) {
    if (shape.size() != 3) return {};
    try {
        return matrix_multiplication_symmetries(field, shape[0], shape[1], shape[2]);
    } catch (const std::exception&) {
        return {};
    }
}

CandidateVerdict tree_verdict(const Field& field, const formats::Tensor& tensor,
                              std::size_t products, const Matrix& candidate,
                              const std::vector<Matrix>& pool,
                              const std::vector<Automorphism>& ambient, std::size_t node_limit,
                              std::vector<Matrix>& decomposition, bool spread_over_cores) {
    const cli::Clock::time_point started = cli::Clock::now();
    CandidateVerdict verdict;

    // `expand_subspace_up_to_symmetry` wants a group that already stabilises the span it is
    // given, and the span here is the map's enlarged by the candidate, so the
    // stabiliser is taken of that. This is the recomputation
    // `methods/bilinear_rank/orbit_reduction/orbit_heuristic.h` explains: a quotient of the map's own
    // group goes stale the moment the object moves, and here it moves once per
    // candidate.
    std::vector<Matrix> subspace = tensor.slices;
    subspace.push_back(candidate);
    const std::vector<Automorphism> group = stabiliser_of(field, subspace, ambient);

    SearchBudget budget(node_limit);
    std::vector<Matrix> found;
    const bool reached =
        expand_subspace_up_to_symmetry(field, subspace, pool, group, products, budget, found,
                                       spread_over_cores);

    verdict.nodes = budget.nodes_visited.load();
    verdict.seconds = cli::elapsed_seconds(started);
    if (reached) {
        verdict.verdict = satisfiability::Verdict::Yes;
        decomposition = std::move(found);
        return verdict;
    }
    // `SearchBudget::tree_fully_walked` starts true and is set false when the node limit is
    // hit, so true means the tree was walked out and a negative answer is a
    // refutation, while false means it gave up. The field name reads the other way
    // round and the prose beside it in `exhaustive_search.h` has the two swapped;
    // the code is what is followed here.
    verdict.verdict = budget.tree_fully_walked.load() ? satisfiability::Verdict::No
                                              : satisfiability::Verdict::Unknown;
    return verdict;
}

}  // namespace bilinear_rank
