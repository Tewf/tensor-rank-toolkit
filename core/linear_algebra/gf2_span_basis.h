#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "gf2_bits.h"

namespace linear_algebra {

/// The GF(2) case of [`SpanBasis`](span_basis.h): the same reduced row echelon
/// form over the same field, with a bit per entry instead of an `int64_t`.
///
/// **It is the same algorithm and not a different one.** Over GF(2) the only
/// nonzero scalar is 1, so `neg` is the identity, the normalisation `try_add`
/// does by inverting the pivot is a no-op, and `axpyin` with a nonzero factor is
/// exclusive or. Dropping all three leaves the loops below, which visit the rows
/// in the same order and pick the same pivots, so a candidate is reduced to zero
/// here exactly when it reduces to zero there.
///
/// What changes is the cost. Reducing a candidate is one XOR per basis row
/// against one field operation per column per basis row: `Θ(d·w/64)` word
/// operations for `Θ(d·w)` calls into Givaro. That ratio, on the leaf test of
/// the exact search, is what
/// [`../positioning/hardware-and-parallelism.md`](../positioning/hardware-and-parallelism.md)
/// measures.
///
/// The rows are one flat block rather than a vector per row, so reducing walks
/// memory forwards and a basis of eleven 5x5 maps is eleven words in one cache
/// line.
class Gf2SpanBasis {
   public:
    explicit Gf2SpanBasis(std::size_t width)
        : width_(width), words_per_row_(gf2_word_count(width)) {}

    std::size_t dimension() const { return pivots_.size(); }
    std::size_t words_per_row() const { return words_per_row_; }

    /// Reduce a packed vector against the rows held. What remains is zero
    /// exactly when the vector was already in the span.
    void reduce(std::uint64_t* entries) const {
        for (std::size_t index = 0; index < pivots_.size(); ++index) {
            if (!gf2_bit(entries, pivots_[index])) continue;
            gf2_xor(entries, &rows_[index * words_per_row_], words_per_row_);
        }
    }

    /// The membership test, reusing the caller's buffer. The exact search asks
    /// this once per pool element per leaf, so the buffer is the caller's for
    /// the same reason it is in `SpanBasis`.
    bool contains(const std::uint64_t* candidate, std::vector<std::uint64_t>& scratch) const {
        scratch.assign(candidate, candidate + words_per_row_);
        reduce(scratch.data());
        return gf2_is_zero(scratch.data(), words_per_row_);
    }

    /// Add the candidate if it is outside the span, and say whether it was.
    bool try_add(const std::uint64_t* candidate, std::vector<std::uint64_t>& scratch) {
        scratch.assign(candidate, candidate + words_per_row_);
        reduce(scratch.data());
        const std::size_t pivot = gf2_lowest_set_bit(scratch.data(), width_);
        if (pivot == width_) return false;

        // Clear the new pivot out of the rows already held, so the set stays
        // reduced and `reduce` does not depend on the order of the rows.
        for (std::size_t index = 0; index < pivots_.size(); ++index) {
            std::uint64_t* row = &rows_[index * words_per_row_];
            if (gf2_bit(row, pivot)) gf2_xor(row, scratch.data(), words_per_row_);
        }

        rows_.insert(rows_.end(), scratch.begin(), scratch.end());
        pivots_.push_back(pivot);
        return true;
    }

   private:
    std::size_t width_;
    std::size_t words_per_row_;
    std::vector<std::uint64_t> rows_;  // `words_per_row_` words each, end to end
    std::vector<std::size_t> pivots_;
};

}  // namespace linear_algebra
