#pragma once

#include <cstddef>
#include <vector>

#include "field.h"

namespace linear_algebra {

/// A basis of a set of vectors, built one candidate at a time and kept in
/// reduced row echelon form.
///
/// Both searches ask "is this one new?" far more often than they ask anything
/// else. Reducing the candidate against what is already held answers this in one
/// pass, which is critical for the search to finish in reasonable time.
template <class Field>
class SpanBasis {
public:
    using Element = typename Field::Element;

    SpanBasis(const Field& field, std::size_t width) : field_(&field), width_(width) {}

    std::size_t dimension() const { return rows_.size(); }

    /// Reduce a vector against the rows held. What remains is zero exactly when
    /// the vector was already in the span. The vector must be as wide as this
    /// span was built (`entries.size() == width`): assumed, not checked, for
    /// the reason matrix.h gives; this is the innermost question both
    /// searches ask.
    void reduce(std::vector<Element>& entries) const {
        for (std::size_t index = 0; index < rows_.size(); ++index) {
            const Element leading = entries[pivot_columns_[index]];
            if (field_->isZero(leading)) continue;
            Element factor;
            field_->neg(factor, leading);
            for (std::size_t column = 0; column < width_; ++column) {
                field_->axpyin(entries[column], factor, rows_[index][column]);
            }
        }
    }

    bool contains(const std::vector<Element>& candidate) const {
        std::vector<Element> entries = candidate;
        reduce(entries);
        for (const Element& entry : entries) {
            if (!field_->isZero(entry)) return false;
        }
        return true;
    }

    bool contains(const MatrixOver<Field>& candidate) const {
        return contains(flatten(candidate));
    }

    /// The same test, reusing the caller's buffer.
    ///
    /// The exact search asks this hundreds of millions of times, once per pool
    /// element per leaf, and the allocation behind the plain overload dominated
    /// everything else it did.
    bool contains(const MatrixOver<Field>& candidate, std::vector<Element>& scratch) const {
        return contains(candidate.data(), candidate.entry_count(), scratch);
    }

    /// The same again over a contiguous run of entries, which is what a matrix
    /// row already is.
    bool contains(const Element* candidate, std::size_t length,
                  std::vector<Element>& scratch) const {
        scratch.assign(candidate, candidate + length);
        reduce(scratch);
        for (const Element& entry : scratch) {
            if (!field_->isZero(entry)) return false;
        }
        return true;
    }

    /// One row, addressed where it lies: the row-space queries ask this per row
    /// of every matrix they compare, and `MatrixOver::row` copies to hand one
    /// over.
    bool contains_row(const MatrixOver<Field>& matrix, std::size_t row,
                      std::vector<Element>& scratch) const {
        return contains(matrix.data() + row * matrix.columns(), matrix.columns(), scratch);
    }

    /// Add the candidate if it is outside the span, and say whether it was.
    bool try_add(const std::vector<Element>& candidate) {
        return try_add(candidate.data(), candidate.size());
    }

    /// The same over a contiguous run, which is what a matrix row already is.
    ///
    /// The copy here is not waste: on success it becomes the row that is kept.
    /// What was waste is the one in front of it, since `Matrix::row` returns a
    /// vector by value and `rank` calls it once per row, so every rank cost two
    /// allocations a row where one is needed.
    bool try_add(const Element* candidate, std::size_t length) {
        std::vector<Element> entries(candidate, candidate + length);
        reduce(entries);

        std::size_t pivot = width_;
        for (std::size_t column = 0; column < width_; ++column) {
            if (!field_->isZero(entries[column])) {
                pivot = column;
                break;
            }
        }
        if (pivot == width_) return false;

        Element scale;
        field_->inv(scale, entries[pivot]);
        for (Element& entry : entries) field_->mulin(entry, scale);

        // Clear the new pivot out of the rows already held, so the set stays
        // reduced and `reduce` does not depend on the order of the rows.
        for (std::vector<Element>& row : rows_) {
            const Element leading = row[pivot];
            if (field_->isZero(leading)) continue;
            Element factor;
            field_->neg(factor, leading);
            for (std::size_t column = 0; column < width_; ++column) {
                field_->axpyin(row[column], factor, entries[column]);
            }
        }

        rows_.push_back(std::move(entries));
        pivot_columns_.push_back(pivot);
        return true;
    }

    bool try_add(const MatrixOver<Field>& candidate) {
        return try_add(candidate.data(), candidate.entry_count());
    }

    /// The basis itself, in echelon form.
    ///
    /// Exposed so a caller can walk the subspace rather than test everything
    /// else for membership in it, which is the cheaper direction whenever
    /// `p^dimension` is smaller than whatever it would otherwise scan.
    const std::vector<std::vector<Element>>& rows() const { return rows_; }

    /// The column each held row leads on, in the same order as `rows()`.
    ///
    /// Exposed for the same reason as `rows()`: a caller reading the basis as a
    /// reduced echelon form needs to know which coordinate each row owns, and
    /// re-deriving it by scanning for the first nonzero is the same fact stored
    /// twice.
    const std::vector<std::size_t>& pivot_columns() const { return pivot_columns_; }

    static std::vector<Element> flatten(const MatrixOver<Field>& matrix) {
        return std::vector<Element>(matrix.data(), matrix.data() + matrix.entry_count());
    }

private:
    const Field* field_;
    std::size_t width_;
    std::vector<std::vector<Element>> rows_;
    std::vector<std::size_t> pivot_columns_;
};

/// The length a slice occupies once flattened into a vector, which is the width
/// every span over those slices has. Zero for an empty set, which is the width
/// of the empty span.
template <class Field>
std::size_t flattened_width(const std::vector<MatrixOver<Field>>& slices) {
    return slices.empty() ? 0 : slices.front().entry_count();
}

/// The span of a set of slices.
///
/// Both searches open with this, and both used to carry a private copy of it.
template <class Field>
SpanBasis<Field> span_of(const Field& field, const std::vector<MatrixOver<Field>>& slices) {
    SpanBasis<Field> span(field, flattened_width<Field>(slices));
    for (const MatrixOver<Field>& slice : slices) span.try_add(slice);
    return span;
}

}  // namespace linear_algebra
