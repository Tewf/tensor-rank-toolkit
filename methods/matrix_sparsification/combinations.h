#pragma once

#include <cstddef>
#include <functional>
#include <vector>

/// Choosing `k` of `n`, which is what the search enumerates over and what its
/// cost is written in.
namespace matrix_sparsification {

/// `C(total, size)`, saturating at `SIZE_MAX` rather than wrapping.
///
/// Exposed because two callers now price a walk before taking it, and a count
/// that wrapped into looking affordable is the one failure a budget cannot
/// survive.
std::size_t subset_count(std::size_t total, std::size_t size);

/// All subsets of `{0, ..., total-1}` of exactly `size`, in lexicographic
/// order. There are `C(total, size)` of them, and an empty result when `size`
/// exceeds `total`.
std::vector<std::vector<std::size_t>> combinations(std::size_t total, std::size_t size);

/// The same subsets in the same order, handed over one at a time.
///
/// The list above is the right shape for a method that walks one fixed size and
/// wants to price it up front. It is the wrong shape for a scan that stops as
/// soon as it has an answer: `C(23, 11)` is 1.3 million subsets and holding them
/// costs 120 MB to look at the first few thousand. Returning false from
/// `visit` stops the walk.
void walk_combinations(std::size_t total, std::size_t size,
                       const std::function<bool(const std::vector<std::size_t>&)>& visit);

}  // namespace matrix_sparsification
