#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "measures.h"

/// The bounds you can have for free, which nothing exponential should ever be
/// asked to rediscover.
///
/// A three-dimensional tensor has three **flattenings**, one per axis, each an
/// ordinary matrix obtained by keeping that axis and merging the other two. A
/// rank-one tensor has rank-one flattenings, and rank is subadditive, so
///
/// > `rank(T) ≥ max_d rank(T⁽ᵈ⁾)`
///
/// **That is derived above and not cited, deliberately.** A rank-one tensor has
/// rank-one flattenings and rank is subadditive, which is the whole proof, so
/// there is no algorithm here from a paper to name; the definition it rests on
/// is `[brockett1978]`'s and keys are [`../references.md`](../references.md).
/// A search for an earlier owner of the inequality itself found the bound
/// treated as folklore everywhere it appears, so nothing is claimed for it.
///
/// It is a lower bound in **polynomial time**, against a decision procedure
/// that is exponential in the rank it is asking about. Asking a solver to rule
/// out a rank the flattenings already forbid is pure waste, and this repository
/// was doing exactly that: a sweep from `k = 1` spent solver calls on ranks that
/// Gaussian elimination refutes in microseconds.
///
/// **Conciseness.** When a flattening is rank deficient the tensor is not
/// concise: it genuinely lives in a smaller space, and every axis can be
/// compressed to its flattening rank without changing the rank. Jason Yang's
/// search (`[yang2025]`; keys are [`../references.md`](../references.md)) does
/// this at *every node* of its recursion, which is a large part of why it is
/// fast. Here it is only detected, by `is_concise`. Compressing the tensor and
/// mapping a decomposition back is the obvious next step and is not done yet, so
/// no caller acts on the answer.
namespace linear_algebra {

/// The `axis`-th flattening: axis 0 keeps rows, axis 1 keeps columns, axis 2
/// keeps slices, and the other two indices are merged into the columns.
template <class Field>
MatrixOver<Field> flattening(const Field& field, const std::vector<MatrixOver<Field>>& slices,
                             std::size_t axis) {
    if (slices.empty()) return MatrixOver<Field>();
    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();
    const std::size_t depth = slices.size();

    const std::size_t kept = axis == 0 ? rows : (axis == 1 ? columns : depth);
    const std::size_t merged = (rows * columns * depth) / std::max<std::size_t>(kept, 1);

    MatrixOver<Field> flat(kept, merged);
    for (std::size_t slice = 0; slice < depth; ++slice) {
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t column = 0; column < columns; ++column) {
                const std::size_t line = axis == 0 ? row : (axis == 1 ? column : slice);
                const std::size_t offset = axis == 0   ? column * depth + slice
                                           : axis == 1 ? row * depth + slice
                                                       : row * columns + column;
                field.assign(flat(line, offset), slices[slice](row, column));
            }
        }
    }
    return flat;
}

/// The three flattening ranks, in axis order.
template <class Field>
std::vector<std::size_t> flattening_ranks(const Field& field,
                                          const std::vector<MatrixOver<Field>>& slices) {
    std::vector<std::size_t> ranks;
    ranks.reserve(3);
    for (std::size_t axis = 0; axis < 3; ++axis) {
        ranks.push_back(rank(field, flattening(field, slices, axis)));
    }
    return ranks;
}

/// `max_d rank(T⁽ᵈ⁾)`: a lower bound on the rank, in polynomial time.
///
/// Sound but weak. Koszul and Young flattenings give better polynomial bounds
/// and are not implemented here.
template <class Field>
std::size_t flattening_lower_bound(const Field& field,
                                   const std::vector<MatrixOver<Field>>& slices) {
    const std::vector<std::size_t> ranks = flattening_ranks(field, slices);
    return *std::max_element(ranks.begin(), ranks.end());
}

/// Whether every axis is already at full rank, so nothing can be compressed
/// away. A tensor that is not concise is smaller than it looks.
template <class Field>
bool is_concise(const Field& field, const std::vector<MatrixOver<Field>>& slices) {
    if (slices.empty()) return true;
    const std::vector<std::size_t> ranks = flattening_ranks(field, slices);
    return ranks[0] == slices.front().rows() && ranks[1] == slices.front().columns() &&
           ranks[2] == slices.size();
}

}  // namespace linear_algebra
