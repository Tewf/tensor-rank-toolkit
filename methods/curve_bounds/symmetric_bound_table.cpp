#include "symmetric_bound_table.h"

namespace curve_bounds {

namespace {

/// One row of `[rambaud2014, Table 1]`, indexed by `m` from 1.
///
/// `{lower, upper}` with `lower == upper` where the value is optimal, `lower ==
/// 0` where only an upper bound is published, and `{0, 0}` for the dots. The
/// three entries the paper sets in bold as its new upper bounds are (3, 2),
/// (2, 4) and (1, 10).
struct Entry {
    std::size_t lower;
    std::size_t upper;
};

constexpr std::size_t kMaxDegree = 10;
constexpr std::size_t kMaxTruncation = 10;

// Rows are l = 1..10, columns m = 1..10.
constexpr Entry kTable[kMaxTruncation][kMaxDegree] = {
    // l = 1: multiplication in GF(2^m) itself.
    {{1, 1}, {3, 3}, {6, 6}, {9, 9}, {13, 13}, {15, 15}, {16, 22}, {0, 24}, {0, 30}, {0, 33}},
    // l = 2
    {{3, 3}, {9, 9}, {0, 16}, {0, 24}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 3
    {{5, 5}, {15, 15}, {0, 30}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 4
    {{8, 8}, {0, 21}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 5
    {{11, 11}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 6
    {{14, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 7
    {{16, 18}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 8
    {{0, 22}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 9
    {{0, 27}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // l = 10
    {{0, 30}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

}  // namespace

Bound symmetric_bound(std::size_t extension_degree, std::size_t truncation) {
    if (extension_degree == 0 || truncation == 0) return Bound();
    if (extension_degree > kMaxDegree || truncation > kMaxTruncation) return Bound();

    const Entry entry = kTable[truncation - 1][extension_degree - 1];
    if (entry.upper == 0) return Bound();

    Bound bound;
    bound.known = true;
    bound.lower = entry.lower;
    bound.upper = entry.upper;
    return bound;
}

std::size_t symmetric_upper_bound(std::size_t extension_degree, std::size_t truncation) {
    return symmetric_bound(extension_degree, truncation).upper;
}

}  // namespace curve_bounds
