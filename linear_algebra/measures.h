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
    // The row by pointer, not by value: `Matrix::row` would allocate a vector
    // for each one and `try_add` copies it anyway.
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        span.try_add(matrix.data() + row * matrix.columns(), matrix.columns());
    }
    return span.dimension();
}

/// Whether a matrix has rank exactly one, decided without computing its rank.
///
/// The exhaustive search asks this of every element of every subspace it walks
/// and never wants the number, so [`rank`](measures.h) above answers a harder
/// question than the caller asked: it builds a `SpanBasis`, copies every row
/// into it and runs the elimination to the last one, where the verdict is
/// settled at the second nonzero row. The matrix arrives here as a pointer and a
/// shape so that no `Matrix` need be formed to ask, and nothing here allocates.
///
/// A matrix has rank one exactly when it has a nonzero row and every row is a
/// scalar multiple of the first nonzero one. **The multiple is never formed**:
/// with `a` the first nonzero row and `p` the column of its first nonzero entry,
/// a row `b` is a multiple of `a` exactly when `a[p]·b[j] = b[p]·a[j]` for every
/// `j`, which is that statement with the division cleared and so costs no
/// inverse. `a[p]` is invertible, which is what makes the two equivalent and is
/// why this is a claim about a field and not about a ring.
///
/// **A zero row satisfies the test with no case of its own**: both sides come
/// out `a[p]·0 = 0·a[j]`, so a zero row is a multiple of `a` and passes, which
/// is the right answer and the one place this would quietly go wrong if the
/// arithmetic were rearranged. Rows before the first nonzero one are zero by
/// construction and need not be revisited. The zero matrix has rank zero, not
/// one, and is refused.
///
/// This and [`gf2_is_rank_one`](gf2_bits.h) are the same predicate over two
/// representations and **must agree about what rank one means**: the exhaustive
/// search sends a leaf down one path or the other on the characteristic alone,
/// so a disagreement would be a rank that depends on which path ran. Over GF(2)
/// the only nonzero scalar is 1, so "a scalar multiple of the first nonzero row"
/// degenerates to "equal to it" and the cross-multiplication collapses into a
/// word comparison. Both refuse the zero matrix, and
/// [`../exhaustive_search/tests/test_rank_one_predicate.cpp`](../exhaustive_search/tests/test_rank_one_predicate.cpp)
/// holds this one against `rank` over every small matrix rather than trusting
/// the argument above.
template <class Field>
bool is_rank_one(const Field& field, const typename Field::Element* data, std::size_t rows,
                 std::size_t columns) {
    using Element = typename Field::Element;
    if (rows == 0 || columns == 0) return false;

    std::size_t leading_row = rows;
    std::size_t pivot = columns;
    for (std::size_t row = 0; row < rows && leading_row == rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            if (field.isZero(data[row * columns + column])) continue;
            leading_row = row;
            pivot = column;
            break;
        }
    }
    if (leading_row == rows) return false;

    const Element* leading = data + leading_row * columns;
    Element ours;
    Element theirs;
    for (std::size_t row = leading_row + 1; row < rows; ++row) {
        const Element* other = data + row * columns;
        for (std::size_t column = 0; column < columns; ++column) {
            field.mul(ours, leading[pivot], other[column]);
            field.mul(theirs, other[pivot], leading[column]);
            if (!field.areEqual(ours, theirs)) return false;
        }
    }
    return true;
}

/// The same predicate on a matrix already formed, for callers that hold one.
template <class Field>
bool is_rank_one(const Field& field, const MatrixOver<Field>& matrix) {
    return is_rank_one(field, matrix.data(), matrix.rows(), matrix.columns());
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
