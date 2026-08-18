#include "minimum_weight_basis.h"

#include <algorithm>

#include "memory_budget.h"
#include "measures.h"
#include "span_basis.h"
#include "span_enumeration.h"

namespace bilinear_rank {

std::vector<Matrix> minimum_weight_basis_with(const Field& field, const std::vector<Matrix>& slices,
                               const Matrix& candidate, const std::vector<std::size_t>& known) {
    std::vector<Matrix> enlarged = slices;
    enlarged.push_back(candidate);
    return minimum_weight_basis(field, enlarged, known);
}

std::vector<std::size_t> span_element_ranks(const Field& field,
                                            const std::vector<Matrix>& slices) {
    const std::size_t combinations = span_size(field, slices.size());
    require_room("the ranks of a span of " + std::to_string(slices.size()) + " slices",
                 combinations, sizeof(std::size_t));

    std::vector<std::size_t> ranks(combinations);
    for (std::size_t index = 0; index < combinations; ++index) {
        ranks[index] = linear_algebra::rank(
            field,
            linear_combination(field, slices, coefficient_vector(index, slices.size(), field.characteristic())));
    }
    return ranks;
}

std::vector<Matrix> minimum_weight_basis(const Field& field, const std::vector<Matrix>& slices,
                                   const std::vector<std::size_t>& ranks_without_last) {
    const std::size_t width = linear_algebra::flattened_width<Field>(slices);
    const std::size_t dimension = linear_algebra::span_of(field, slices).dimension();
    const std::size_t combinations = span_size(field, slices.size());

    // Every element of the span, cheapest first. Index 0 is the zero
    // combination and is skipped: it can never enter a basis.
    //
    // Only the rank and the index are held. The element itself is rebuilt from
    // its index by `linear_combination` when the greedy actually reaches it, which is at
    // most `dimension` times. Holding the matrices instead costs
    // `p^slices * (56 + 8*n*m)` bytes: 134 MB for the sixteen slices of 4x4
    // matrix multiplication, against 1 MB this way.
    struct Candidate {
        std::size_t rank;
        std::size_t index;
    };
    require_room("the span of " + std::to_string(slices.size()) + " slices",
                 combinations - 1, sizeof(Candidate));

    std::vector<Candidate> candidates;
    candidates.reserve(combinations - 1);
    for (std::size_t index = 1; index < combinations; ++index) {
        if (index < ranks_without_last.size()) {
            candidates.push_back({ranks_without_last[index], index});
            continue;
        }
        const Matrix element =
            linear_combination(field, slices, coefficient_vector(index, slices.size(), field.characteristic()));
        candidates.push_back({linear_algebra::rank(field, element), index});
    }

    // Sort by rank, ties broken by enumeration order, to ensure reproducible
    // results.
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& left, const Candidate& right) {
                  if (left.rank != right.rank) return left.rank < right.rank;
                  return left.index < right.index;
              });

    std::vector<Matrix> basis;
    ReducedBasis span(field, width);
    for (const Candidate& candidate : candidates) {
        if (basis.size() == dimension) break;
        Matrix element = linear_combination(
            field, slices, coefficient_vector(candidate.index, slices.size(), field.characteristic()));
        if (span.try_add(element)) {
            basis.push_back(std::move(element));
        }
    }
    return basis;
}

}  // namespace bilinear_rank
