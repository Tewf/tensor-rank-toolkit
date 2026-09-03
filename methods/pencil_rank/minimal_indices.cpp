#include "minimal_indices.h"

#include <stdexcept>
#include <string>

#include "matrix_ops.h"
#include "measures.h"

namespace pencil_rank {

namespace {

/// The block system whose null space is the pencil's null vectors of degree
/// below `depth`, as drawn in the header. `depth + 1` block rows of `first` on
/// the diagonal and `second` one block below it.
ModularMatrix shift_system(const ModularMatrix& first, const ModularMatrix& second,
                           std::size_t depth) {
    const std::size_t rows = first.rows();
    const std::size_t columns = first.columns();
    ModularMatrix system((depth + 1) * rows, depth * columns);
    for (std::size_t block = 0; block < depth; ++block) {
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t column = 0; column < columns; ++column) {
                system(block * rows + row, block * columns + column) = first(row, column);
                system((block + 1) * rows + row, block * columns + column) = second(row, column);
            }
        }
    }
    return system;
}

}  // namespace

std::vector<std::size_t> column_minimal_indices(const ModularField& field,
                                                const ModularMatrix& first,
                                                const ModularMatrix& second,
                                                std::size_t expected_count) {
    std::vector<std::size_t> indices;
    if (expected_count == 0) return indices;

    const std::size_t columns = first.columns();

    // `nullity[depth]` is the count of null vectors of degree below `depth`,
    // and `found_below` its first difference: how many indices are under
    // `depth`. The second difference is how many equal `depth - 1`.
    std::vector<std::size_t> nullity(columns + 2, 0);
    std::size_t found_below = 0;

    for (std::size_t depth = 1; depth <= columns; ++depth) {
        const ModularMatrix system = shift_system(first, second, depth);
        nullity[depth] = depth * columns - linear_algebra::rank(field, system);

        const std::size_t below = nullity[depth] - nullity[depth - 1];
        for (std::size_t repeat = found_below; repeat < below; ++repeat) {
            indices.push_back(depth - 1);
        }
        found_below = below;

        // Every index is accounted for, and the systems only get larger, so
        // there is nothing left to learn from a deeper one.
        if (indices.size() == expected_count) return indices;
    }

    throw std::runtime_error("pencil_rank: found " + std::to_string(indices.size()) +
                             " minimal indices where the pencil's null space over GF(p)(x) has "
                             "dimension " + std::to_string(expected_count));
}

std::vector<std::size_t> row_minimal_indices(const ModularField& field,
                                             const ModularMatrix& first,
                                             const ModularMatrix& second,
                                             std::size_t expected_count) {
    return column_minimal_indices(field, linear_algebra::transpose<ModularField>(first),
                                  linear_algebra::transpose<ModularField>(second), expected_count);
}

}  // namespace pencil_rank
