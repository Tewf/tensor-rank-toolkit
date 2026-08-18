#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"

/// Walking the span of a bilinear map, element by element.
///
/// A map with `k` slices over `GF(p)` spans `p^k` maps, and step 1 has to see
/// all of them to pick the cheapest basis. Enumerating them is a separate job
/// from choosing among them, and it is the one that decides how far the search
/// can scale: the span is materialised, so memory runs out before time does.
namespace bilinear_rank {

/// The linear combination `sum_i coefficients[i] * slices[i]`, reduced.
Matrix linear_combination(const Field& field, const std::vector<Matrix>& slices,
               const std::vector<int64_t>& coefficients);

/// The coefficient vector at `index`, counting with the first coordinate
/// varying fastest. This enumeration order is fixed and deterministic so that
/// the tie-break between equal-rank candidates is consistent across runs, which
/// is load-bearing for reproducibility.
std::vector<int64_t> coefficient_vector(std::size_t index, std::size_t count,
                                        int64_t characteristic);

/// How many elements the span has, which is how many combinations must be
/// enumerated to see all of it. Throws rather than return a number no machine
/// could enumerate.
std::size_t span_size(const Field& field, std::size_t slice_count);

}  // namespace bilinear_rank
