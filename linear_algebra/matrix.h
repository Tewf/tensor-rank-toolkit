#pragma once

#include <cstddef>
#include <vector>

namespace linear_algebra {

/// A dense row-major matrix of field elements.
///
/// Small by construction: the maps searched over here are a few dozen entries,
/// so there is nothing to gain from a sparse representation and a great deal to
/// lose in legibility. The element type is what makes the same code serve both
/// a finite field and the rationals.
template <class Element>
class Matrix {
public:
    Matrix() = default;
    Matrix(std::size_t rows, std::size_t columns)
        : rows_(rows), columns_(columns), entries_(rows * columns) {}

    std::size_t rows() const { return rows_; }
    std::size_t columns() const { return columns_; }
    std::size_t entry_count() const { return entries_.size(); }

    Element& operator()(std::size_t row, std::size_t column) {
        return entries_[row * columns_ + column];
    }
    const Element& operator()(std::size_t row, std::size_t column) const {
        return entries_[row * columns_ + column];
    }

    Element* data() { return entries_.data(); }
    const Element* data() const { return entries_.data(); }

    std::vector<Element> row(std::size_t index) const {
        return std::vector<Element>(entries_.begin() + static_cast<std::ptrdiff_t>(index * columns_),
                                    entries_.begin() +
                                        static_cast<std::ptrdiff_t>((index + 1) * columns_));
    }

private:
    std::size_t rows_ = 0;
    std::size_t columns_ = 0;
    std::vector<Element> entries_;
};

}  // namespace linear_algebra
