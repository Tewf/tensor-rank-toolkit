#pragma once

#include <string>
#include <utility>
#include <vector>

#include "exhaustive_search.h"
#include "bilinear_rank_aliases.h"

/// Which `k` to ask about, given a search that can decide one `k` at a time.
///
/// [`exhaustive_search.h`](exhaustive_search.h) answers "is there an algorithm
/// with exactly this many products?". Turning that into "how few products are
/// there?" is a separate decision with a separate cost, and three answers to it
/// live here: sweep upward, bisect, or build from nothing.
///
/// All three now start at the flattening lower bound rather than at the span
/// dimension, so no exponential search is ever asked about a `k` that
/// [`tensor_flattening.h`](../linear_algebra/tensor_flattening.h) refutes in
/// microseconds.
namespace bilinear_rank {

/// The smallest `k` worth asking about: the largest lower bound
/// [`rank_lower_bound`](../linear_algebra/rank_lower_bound.h) can prove without
/// searching, which is the flattening ranks and `[yang2025]`'s rank sum.
///
/// Reported by the commands so a run says how much room is left between what a
/// finder reached and what nothing can go below.
std::size_t flattening_floor(const Field& field, const std::vector<Matrix>& base);

/// `10 products, rank bound 8, gap 2`: the sentence every finder ends
/// with, and the room a better one still has above the bound.
///
/// Throws rather than clamping when `products_found` is below `bound`. The two
/// contradict each other there and both are checkable claims, so the run is
/// reporting a bug; a negative gap printed as zero would bury the one failure
/// nothing downstream catches, a lower bound that is not one. Every command
/// turns a thrown refusal into a line and exits non-zero.
std::string require_bound_consistent(std::size_t products_found, std::size_t bound);

/// Sweep `k` upward from `flattening_floor`. The first hit is the fewest products
/// reachable from `base`.
///
/// Trustworthy but slow: it makes no assumption about how the answer behaves
/// in `k`.
bool fewest_products_by_sweep(const Field& field, const std::vector<Matrix>& base,
                              const std::vector<Matrix>& pool, SearchBudget& budget,
                              std::vector<Matrix>& products);

}  // namespace bilinear_rank
