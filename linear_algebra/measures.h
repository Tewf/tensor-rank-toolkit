#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "span_basis.h"

/// What an object costs, in the three currencies this repository counts:
/// multiplications, the nonzero entries that become additions, and the entries
/// that are additions no longer.
namespace linear_algebra {

/// Exact rank: the number of independent rows.
///
/// Computed using a basis walk that reduces modulo p at each step, avoiding
/// overflow and maintaining exact arithmetic throughout.
template <class Field>
std::size_t rank(const Field& field, const MatrixOver<Field>& matrix) {
    if (matrix.rows() == 0 || matrix.columns() == 0) return 0;
    SpanBasis<Field> span(field, matrix.columns());
    for (std::size_t row = 0; row < matrix.rows(); ++row) span.try_add(matrix.row(row));
    return span.dimension();
}

/// The number of multiplications a set of slices costs, which is what the rank
/// search exists to reduce: the sum of their ranks.
template <class Field>
std::size_t multiplication_count(const Field& field,
                                 const std::vector<MatrixOver<Field>>& slices) {
    std::size_t total = 0;
    for (const MatrixOver<Field>& slice : slices) total += rank(field, slice);
    return total;
}

/// How many entries are not zero, which is what the sparsification search
/// exists to reduce.
///
/// The count uses field equality testing, not floating-point rounding, so every
/// zero is definitive and the reported count matches the quantity being
/// optimised.
template <class Field>
std::size_t nonzero_count(const Field& field, const MatrixOver<Field>& matrix) {
    std::size_t total = 0;
    for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
        if (!field.isZero(matrix.data()[entry])) ++total;
    }
    return total;
}

/// How many entries are neither zero nor plus or minus one: the entries that
/// cost a multiplication rather than an addition.
///
/// The papers call these the non-singular values and minimise `nnz + nns`,
/// where this repository has always minimised `nnz` alone. The difference is
/// real: multiplying by 1/2 and adding are not the same instruction, so an
/// operator with fewer nonzeros can still be the more expensive one. Over
/// GF(2) every nonzero entry is one and this is always zero, which is why it
/// only ever mattered on the rational side.
template <class Field>
std::size_t nonsingular_count(const Field& field, const MatrixOver<Field>& matrix) {
    std::size_t total = 0;
    for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
        const typename Field::Element& value = matrix.data()[entry];
        if (field.isZero(value) || field.isOne(value) || field.isMOne(value)) continue;
        ++total;
    }
    return total;
}

/// What the operator costs to apply: one addition per nonzero, one
/// multiplication per entry that is not a sign.
///
/// This is the quantity the sparsification papers minimise, and the one to
/// compare two operators by when their nonzero counts are close.
template <class Field>
std::size_t operation_count(const Field& field, const MatrixOver<Field>& matrix) {
    return nonzero_count(field, matrix) + nonsingular_count(field, matrix);
}

}  // namespace linear_algebra
