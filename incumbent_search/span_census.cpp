#include "span_census.h"

#include <algorithm>

#include "subspace_canon.h"

namespace bilinear_rank {

void SpanTally::record(const Field& field, const std::vector<Matrix>& basis) {
    ++tally_[subspace_code(field, basis)];
    ++recorded_;
}

std::size_t SpanTally::most_repeated() const {
    std::size_t most = 0;
    for (const auto& entry : tally_) most = std::max(most, entry.second);
    return most;
}

}  // namespace bilinear_rank
