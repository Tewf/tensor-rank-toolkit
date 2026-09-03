#pragma once

#include <algorithm>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

/// The two things every check on a canonical form needs: a shape to build the group
/// and the pool from, and subsets of that pool to ask about.
///
/// Named once here because four test programs want them, and a second copy of "how
/// a random subset is drawn" is a copy free to drift from the first: this matters
/// more than it looks, since a differential test that drew its sets differently
/// from the separation test beside it would be silent about the sets that one
/// covers.
namespace pool_sets {

/// The three dimensions of `⟨rows, inner, columns⟩`, which name the group. The
/// slice shape the pool is built from follows from them:
/// `map_construction.h` builds `⟨n,m,k⟩` as `n*k` slices of `(n*m) x (m*k)`.
struct MatmulShape {
    std::size_t rows = 0;
    std::size_t inner = 0;
    std::size_t columns = 0;

    std::size_t slice_rows() const { return rows * inner; }
    std::size_t slice_columns() const { return inner * columns; }
    std::string name() const {
        return "<" + std::to_string(rows) + "," + std::to_string(inner) + "," +
               std::to_string(columns) + ">";
    }
};

/// A subset of the pool of size `wanted`, drawn without replacement and sorted,
/// since that is the shape `pool_inside` returns.
///
/// Rejection rather than a shuffle of the whole pool: `⟨3,3,3⟩`'s is 261 121 long
/// and `⟨4,4,4⟩`'s does not exist, and a set of a dozen indices should not need
/// either of them laid out.
inline std::vector<std::size_t> random_set(std::mt19937& source, std::size_t pool_size,
                                           std::size_t wanted) {
    std::uniform_int_distribution<std::size_t> anywhere(0, pool_size - 1);
    std::vector<std::size_t> indices;
    while (indices.size() < wanted) {
        const std::size_t drawn = anywhere(source);
        if (std::find(indices.begin(), indices.end(), drawn) == indices.end()) {
            indices.push_back(drawn);
        }
    }
    std::sort(indices.begin(), indices.end());
    return indices;
}

}  // namespace pool_sets
