#include "sorted_span.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "measures.h"
#include "memory_budget.h"
#include "span_basis.h"
#include "span_enumeration.h"

namespace bilinear_rank {

SortedSpan::SortedSpan(const Field& field, const std::vector<Matrix>& slices,
                       const std::vector<std::size_t>& ranks_without_last) {
    if (slices.empty()) return;
    const std::size_t width = linear_algebra::flattened_width<Field>(slices);
    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();
    const std::size_t highest = std::min(rows, columns);
    const std::size_t combinations = span_size(field, slices.size());
    const auto characteristic = field.characteristic();
    dimension_ = linear_algebra::span_of(field, slices).dimension();

    // One byte an element, against the sixteen a `{rank, index}` pair costs.
    // Nothing else is held: the element itself is rebuilt from its index when
    // the greedy below reaches it, which is at most `dimension` times.
    require_room("the ranks of a span of " + std::to_string(slices.size()) + " slices",
                 combinations, sizeof(std::uint8_t));
    std::vector<std::uint8_t> rank_of(combinations, 0);
    std::vector<int64_t> coefficients;
    Matrix combination;
    for (std::size_t index = 1; index < combinations; ++index) {
        if (index < ranks_without_last.size()) {
            rank_of[index] = static_cast<std::uint8_t>(ranks_without_last[index]);
            continue;
        }
        coefficient_vector_into(index, slices.size(), characteristic, coefficients);
        linear_combination_into(field, slices, coefficients, combination);
        rank_of[index] = static_cast<std::uint8_t>(linear_algebra::rank(field, combination));
    }

    // The greedy, with the sort replaced by a pass per rank. Ascending rank and,
    // within a rank, ascending index: the same order the comparison sort
    // produced, so the basis this walks is the basis that one walked.
    ReducedBasis span(field, width);
    reached_.reserve(highest);
    for (std::size_t rank = 1; rank <= highest; ++rank) {
        for (std::size_t index = 1; index < combinations; ++index) {
            if (rank_of[index] != rank) continue;
            if (span.dimension() == dimension_) break;
            coefficient_vector_into(index, slices.size(), characteristic, coefficients);
            linear_combination_into(field, slices, coefficients, combination);
            span.try_add(combination);
        }
        reached_.push_back(span.dimension());
    }
}

std::size_t SortedSpan::reached_by_rank(std::size_t rank) const {
    if (rank == 0) return 0;
    if (rank > reached_.size()) return dimension_;
    return reached_[rank - 1];
}

std::size_t SortedSpan::cost() const {
    std::size_t total = 0;
    for (std::size_t rank = 1; rank <= reached_.size(); ++rank) {
        total += rank * (reached_by_rank(rank) - reached_by_rank(rank - 1));
    }
    return total;
}

}  // namespace bilinear_rank
