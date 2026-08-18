#pragma once

#include <cstddef>
#include <vector>

/// Choosing `k` of `n`, which is what both oracles enumerate over and what
/// their cost is written in.
namespace matrix_sparsification {

/// All subsets of `{0, ..., total-1}` of exactly `size`, in lexicographic
/// order. There are `C(total, size)` of them, and an empty result when `size`
/// exceeds `total`.
std::vector<std::vector<std::size_t>> combinations(std::size_t total, std::size_t size);

}  // namespace matrix_sparsification
