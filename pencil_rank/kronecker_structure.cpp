#include "kronecker_structure.h"

#include <stdexcept>
#include <string>

#include "minimal_indices.h"

namespace pencil_rank {

namespace {

std::size_t sum_of(const std::vector<std::size_t>& values) {
    std::size_t total = 0;
    for (const std::size_t value : values) total += value;
    return total;
}

/// The size the KCF's regular block must have for the block sizes to add up to
/// `extent`. Both axes give it, and `kronecker_structure` requires the answers
/// to agree.
///
/// Along the columns an `L` block of index `e` is `e + 1` wide and an `L`
/// transpose block of index `e` is `e` wide; along the rows the two swap. The
/// subtraction is checked rather than trusted, because an underflow here would
/// wrap into an enormous regular part and every later number would follow it.
std::size_t regular_size_from(std::size_t extent, const std::vector<std::size_t>& wide,
                              const std::vector<std::size_t>& narrow, const char* axis) {
    const std::size_t occupied = sum_of(wide) + wide.size() + sum_of(narrow);
    if (occupied > extent) {
        throw std::runtime_error(std::string("pencil_rank: the singular blocks take ") +
                                 std::to_string(occupied) + " of " + std::to_string(extent) +
                                 " along the " + axis);
    }
    return extent - occupied;
}

}  // namespace

KroneckerStructure kronecker_structure(const ModularField& field, const ModularMatrix& first,
                                       const ModularMatrix& second) {
    KroneckerStructure structure;

    const PencilDivisors divisors = elementary_divisors(field, first, second);
    structure.divisors = divisors.divisors;

    // A pencil of rank `generic_rank` over GF(p)(x) has that much null space
    // left on each side, and each null direction is one minimal index.
    structure.column_indices =
        column_minimal_indices(field, first, second, first.columns() - divisors.generic_rank);
    structure.row_indices =
        row_minimal_indices(field, first, second, first.rows() - divisors.generic_rank);

    const std::size_t by_columns = regular_size_from(first.columns(), structure.column_indices,
                                                     structure.row_indices, "columns");
    const std::size_t by_rows = regular_size_from(first.rows(), structure.row_indices,
                                                  structure.column_indices, "rows");
    if (by_columns != by_rows) {
        throw std::runtime_error("pencil_rank: the regular part is " + std::to_string(by_columns) +
                                 " counted along the columns and " + std::to_string(by_rows) +
                                 " counted along the rows");
    }

    std::size_t spanned = 0;
    for (const ElementaryDivisor& divisor : structure.divisors) spanned += divisor.factor.degree();
    if (spanned != by_columns) {
        throw std::runtime_error("pencil_rank: elementary divisors of total degree " +
                                 std::to_string(spanned) + " for a regular part of size " +
                                 std::to_string(by_columns));
    }

    structure.regular_size = by_columns;
    return structure;
}

std::size_t rank_over_closure(const KroneckerStructure& structure) {
    std::size_t rank = structure.regular_size;

    // A zero index is a row or column that is zero in both slices. It occupies
    // one line of the canonical form and costs nothing, so it is the one block
    // whose size is not its contribution.
    for (const std::size_t index : structure.column_indices) {
        if (index > 0) rank += index + 1;
    }
    for (const std::size_t index : structure.row_indices) {
        if (index > 0) rank += index + 1;
    }

    // Over the closure a divisor `p^e` becomes `deg(p)` divisors `(x - lambda)^e`,
    // each nonlinear exactly when `e >= 2`. So a repeated root costs its base
    // degree and a bare irreducible costs nothing at all: it splits.
    for (const ElementaryDivisor& divisor : structure.divisors) {
        if (divisor.factor.multiplicity >= 2) rank += divisor.factor.base_degree;
    }
    return rank;
}

/// The same sum, counting a divisor as nonlinear when it is nonlinear over
/// GF(p). See `PencilRank::over_the_field` for what is and is not claimed.
std::size_t rank_over_the_field(const KroneckerStructure& structure) {
    std::size_t rank = structure.regular_size;
    for (const std::size_t index : structure.column_indices) {
        if (index > 0) rank += index + 1;
    }
    for (const std::size_t index : structure.row_indices) {
        if (index > 0) rank += index + 1;
    }
    for (const ElementaryDivisor& divisor : structure.divisors) {
        if (!divisor.factor.is_linear()) ++rank;
    }
    return rank;
}

namespace {

/// Whether the pencil is simultaneously diagonalisable over GF(p), which is the
/// case where the closure bound is attained here and provably so.
bool diagonalisable_over_the_field(const KroneckerStructure& structure) {
    if (!structure.column_indices.empty() || !structure.row_indices.empty()) return false;
    for (const ElementaryDivisor& divisor : structure.divisors) {
        if (!divisor.factor.is_linear()) return false;
    }
    return true;
}

}  // namespace

PencilRank pencil_rank_of(const ModularField& field, const std::vector<ModularMatrix>& slices) {
    if (slices.empty()) return PencilRank{0, 0, true};
    if (slices.size() > 2) {
        throw std::runtime_error("pencil_rank: " + std::to_string(slices.size()) +
                                 " slices is not a pencil");
    }

    // One slice is the pencil whose second matrix is zero, and the formula
    // returns that matrix's rank without being told the case is special.
    const ModularMatrix second = slices.size() == 2
                                     ? slices[1]
                                     : ModularMatrix(slices[0].rows(), slices[0].columns());
    const KroneckerStructure structure = kronecker_structure(field, slices[0], second);
    return PencilRank{rank_over_closure(structure), rank_over_the_field(structure),
                      diagonalisable_over_the_field(structure)};
}

}  // namespace pencil_rank
