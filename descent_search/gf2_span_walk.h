#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "gf2_bits.h"
#include "gf2_span_basis.h"
#include "reflected_gray_walk.h"

/// Step 1's span walk over GF(2), with a bit per entry.
///
/// [`minimum_weight_basis.h`](minimum_weight_basis.h) says what the walk is for
/// and this file says nothing about that. It is the same enumeration of the
/// same span in the same order, with a matrix held as bits in machine words
/// instead of as one `int64_t` an entry. It is the precedent
/// [`../exhaustive_search/gf2_leaf.h`](../exhaustive_search/gf2_leaf.h) set for
/// the leaf of the exact search, applied to the two functions the incumbent
/// search spends its life inside.
///
/// **Why here and not somewhere cheaper.** `lower-the-bound` on the 15x12x20
/// flattening of `<3,4,5>` over GF(2) costs one `span_element_ranks` and one
/// `minimum_weight_basis_with` per child; 26 040 children were costed at a
/// single node at `--summand-rank 4`, and each call walks a span of 2^15
/// elements taking the rank of a 12x20 matrix at every one. That is of the
/// order of 850 million ranks for one node, so what a matrix is made of is what
/// the search costs.
///
/// **It is a case chosen at run time, not a build.** The characteristic comes
/// off the tensor file, so `gf2_span_walk_applies` is asked once per call and
/// the general path is what answers when it says no. Nothing over GF(3), GF(5)
/// or the rationals reaches this file, and
/// [`tests/test_gf2_span_walk.cpp`](tests/test_gf2_span_walk.cpp) is what holds
/// the two paths to one answer.
///
/// **Nothing here is a second bit representation.**
/// [`../linear_algebra/gf2_bits.h`](../linear_algebra/gf2_bits.h) already had
/// the packing, the exclusive or and the row reader, and
/// [`../linear_algebra/gf2_span_basis.h`](../linear_algebra/gf2_span_basis.h)
/// already had the reduced row echelon form the greedy asks for. One primitive
/// was missing and was added there rather than here: `gf2_rank`, because the
/// leaf only ever needed to know whether a rank was one, and step 1 needs the
/// number.
///
/// **Four things are packed, and that is the whole file.** The walk itself, the
/// rank of the element it stands on, the dimension the greedy must reach, and
/// the greedy's own membership test. What is left in
/// [`minimum_weight_basis.cpp`](minimum_weight_basis.cpp) is representation
/// free: the rank ceiling, the floor under the unranked half, the sort and the
/// order the greedy takes candidates in are one piece of code that both paths
/// run, which is why a packed run and a general run cannot drift apart on
/// policy.
namespace bilinear_rank {

/// Whether the span walk has a GF(2) form for this field and these slices.
///
/// The width limit is [`gf2_row`](../linear_algebra/gf2_bits.h)'s, which
/// `gf2_rank` reads a row through: a row is the low bits of one word, so a
/// slice may have any number of rows and at most 64 columns. The 12x20 slices
/// of `<3,4,5>` and the 16x16 slices of `<4,4,4>` are both well inside it.
///
/// Answers false for every field and shape while the walk is switched off
/// below, so one switch turns the whole file off rather than each caller
/// remembering to ask twice.
bool gf2_span_walk_applies(const Field& field, const std::vector<Matrix>& slices);

/// Whether this file is offered at all. True unless a run asks otherwise.
///
/// **It exists so the two routes can be timed on the same question**, which is
/// the reason `set_gf2_leaf_offered` exists for the leaf: a path that is only
/// ever taken cannot be compared with the one it replaced, and a comparison
/// across two different questions is not a comparison. So `--general-span` on
/// `lower-the-bound` sets this false for the run, the general path answers
/// every call, every count is unchanged because only the arithmetic differs,
/// and the two wall clocks are of the same tree.
///
/// It is a process-wide setting like `set_worker_count` and
/// `set_memory_budget`, read once per call rather than once per element, and
/// nothing but a command line and the test writes it.
void set_gf2_span_walk_offered(bool offered);
bool gf2_span_walk_offered();

class Gf2GreedyBasis;

/// The span walked one slice at a time, in bits.
///
/// The GF(2) case of `SpanElements` in
/// [`minimum_weight_basis.cpp`](minimum_weight_basis.cpp), and it walks under
/// the same [`ReflectedGrayWalk`](reflected_gray_walk.h) rather than a private
/// copy of that order: consecutive elements differ by one slice, so a step is
/// one exclusive or over `width / 64` words where the general path spends
/// `width` Givaro additions, and the index is carried alongside as the general
/// walk carries it. Over GF(2) a digit moving up and a digit moving down are
/// the same exclusive or, so only the carried index tells the two apart.
class Gf2SpanElements {
   public:
    using Greedy = Gf2GreedyBasis;

    Gf2SpanElements(const Field&, const std::vector<Matrix>& slices)
        : rows_(slices.front().rows()),
          columns_(slices.front().columns()),
          width_(rows_ * columns_),
          words_(linear_algebra::gf2_word_count(width_)),
          count_(slices.size()),
          slices_(slices.size() * words_),
          element_(words_, 0),
          walk_(slices.size(), 2) {
        for (std::size_t slice = 0; slice < count_; ++slice) {
            linear_algebra::gf2_pack(slices[slice].data(), width_, &slices_[slice * words_]);
        }
    }

    /// Step to the next element, or say the span is used up. The index of the
    /// element now held is `index()`.
    bool advance() {
        ReflectedGrayWalk::Step step;
        if (!walk_.advance(step)) return false;

        // The digit that moved, as a place value, which is the `p^digit` the
        // general walk adds and subtracts.
        const std::size_t place = std::size_t(1) << step.digit;
        if (step.upward) {
            index_ += place;
        } else {
            index_ -= place;
        }
        linear_algebra::gf2_xor(element_.data(), &slices_[step.digit * words_], words_);
        return true;
    }

    std::size_t index() const { return index_; }

    /// The rank of the element the walk stands on, which is what the whole
    /// enumeration is for.
    std::size_t rank() const { return linear_algebra::gf2_rank(element_.data(), rows_, columns_); }

    /// The rank of one slice: what the ceiling and the floor are made of.
    std::size_t rank_of_slice(std::size_t slice) const {
        return linear_algebra::gf2_rank(&slices_[slice * words_], rows_, columns_);
    }

    /// How many elements a basis of this span holds, which is where the greedy
    /// stops. `Gf2SpanBasis` picks the pivots `SpanBasis` picks, so this is the
    /// dimension the general path computes.
    std::size_t dimension() const {
        linear_algebra::Gf2SpanBasis span(width_);
        std::vector<std::uint64_t> scratch(words_);
        for (std::size_t slice = 0; slice < count_; ++slice) {
            span.try_add(&slices_[slice * words_], scratch);
        }
        return span.dimension();
    }

    /// What the greedy reads. The slices are packed once, here, so rebuilding
    /// an element from its index is an exclusive or per set bit and no
    /// conversion at all.
    std::size_t rows() const { return rows_; }
    std::size_t columns() const { return columns_; }
    std::size_t width() const { return width_; }
    std::size_t words() const { return words_; }
    std::size_t slice_count() const { return count_; }
    const std::uint64_t* slice_words(std::size_t slice) const { return &slices_[slice * words_]; }

   private:
    std::size_t rows_;
    std::size_t columns_;
    std::size_t width_;
    std::size_t words_;
    std::size_t count_;
    /// Every slice, `words_` words each, end to end.
    std::vector<std::uint64_t> slices_;
    std::vector<std::uint64_t> element_;
    ReflectedGrayWalk walk_;
    std::size_t index_ = 0;
};

/// The basis the greedy is assembling, in bits.
///
/// The GF(2) case of a `ReducedBasis` and of the `linear_combination` that
/// feeds it. An element is rebuilt from the set bits of its index, which over
/// GF(2) are the coefficients `coefficient_vector` hands the general path, and
/// the membership test is `Gf2SpanBasis`'s, which accepts exactly what
/// `SpanBasis` accepts. So the greedy takes the same candidates in the same
/// order and hands back the same basis.
class Gf2GreedyBasis {
   public:
    explicit Gf2GreedyBasis(const Gf2SpanElements& span)
        : span_(span), held_(span.width()), combination_(span.words()), scratch_(span.words()) {}

    /// The element at `index`, taken if it is outside what is already held.
    /// Written into `element` only when it is taken, which is at most
    /// `dimension` times a call.
    bool take(std::size_t index, Matrix& element) {
        for (std::uint64_t& word : combination_) word = 0;
        for (std::size_t slice = 0; slice < span_.slice_count(); ++slice) {
            if (((index >> slice) & 1) == 0) continue;
            linear_algebra::gf2_xor(combination_.data(), span_.slice_words(slice), span_.words());
        }
        if (!held_.try_add(combination_.data(), scratch_)) return false;

        element = Matrix(span_.rows(), span_.columns());
        linear_algebra::gf2_unpack(combination_.data(), span_.width(), element.data());
        return true;
    }

   private:
    const Gf2SpanElements& span_;
    linear_algebra::Gf2SpanBasis held_;
    std::vector<std::uint64_t> combination_;
    std::vector<std::uint64_t> scratch_;
};

}  // namespace bilinear_rank
