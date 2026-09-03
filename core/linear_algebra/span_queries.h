#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "span_basis.h"

/// The questions a search asks about spans, and the invariants it is held to.
///
/// A search that quietly loses a slice reports excellent numbers, so these are
/// checked in the tools themselves and not only in the tests.
namespace linear_algebra {

/// Whether every one of `targets` lies in the span of `spanning_set`.
///
/// This is what makes a rewrite a rewrite rather than a different, cheaper map.
template <class Field>
bool spans_all(const Field& field, const std::vector<MatrixOver<Field>>& spanning_set,
               const std::vector<MatrixOver<Field>>& targets) {
    if (targets.empty()) return true;
    if (spanning_set.empty()) return false;
    SpanBasis<Field> span(field, spanning_set.front().entry_count());
    for (const MatrixOver<Field>& element : spanning_set) span.try_add(element);
    std::vector<typename Field::Element> scratch;
    for (const MatrixOver<Field>& target : targets) {
        if (!span.contains(target, scratch)) return false;
    }
    return true;
}

/// Whether two matrices have the same row space.
///
/// Sparsifying an operator means rewriting it as U V for an invertible V, which
/// on the transposed form is exactly replacing rows by other vectors spanning
/// the same space. A search that returned something genuinely sparser but no
/// longer equivalent would otherwise look like a triumph.
template <class Field>
bool same_row_space(const Field& field, const MatrixOver<Field>& left,
                    const MatrixOver<Field>& right) {
    if (left.columns() != right.columns()) return false;

    SpanBasis<Field> left_span(field, left.columns());
    for (std::size_t row = 0; row < left.rows(); ++row) left_span.try_add(left.row(row));
    SpanBasis<Field> right_span(field, right.columns());
    for (std::size_t row = 0; row < right.rows(); ++row) right_span.try_add(right.row(row));
    if (left_span.dimension() != right_span.dimension()) return false;

    std::vector<typename Field::Element> scratch;
    for (std::size_t row = 0; row < right.rows(); ++row) {
        if (!left_span.contains_row(right, row, scratch)) return false;
    }
    for (std::size_t row = 0; row < left.rows(); ++row) {
        if (!right_span.contains_row(left, row, scratch)) return false;
    }
    return true;
}

}  // namespace linear_algebra
