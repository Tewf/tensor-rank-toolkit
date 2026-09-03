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
/// [`../exhaustive/gf2_leaf.h`](../exhaustive/gf2_leaf.h) set for
/// the leaf of the exact search, applied to the two functions the incumbent
/// search spends its life inside.
///
/// **Why here and not somewhere cheaper.** `tighten-rank-bound` on the 15x12x20
/// flattening of `<3,4,5>` over GF(2) costs one `span_element_ranks` and one
/// `minimum_weight_basis_with` per child, and **one node of it costs 26 040
/// children** at `--summand-rank 4`. The root basis there has dimension 15, so
/// each of those calls walks the 2^16 elements of the span with the candidate
/// adjoined and takes the rank of a 12x20 matrix on the 2^15 of them the
/// handed-over ranks do not cover. That is of the order of 850 million ranks
/// for a single node, so what a matrix is made of is what the search costs.
/// Measured in the last row of the table below: that one node was 233.93 s and
/// is 24.31 s.
///
/// ## What it is worth, measured
///
/// Every pair below is **one question asked twice**, `--general-span` selecting
/// the path this file replaces, so the two columns are one tree:
///
///     build/methods/bilinear_rank/branch_and_bound/tighten-rank-bound FIXTURE --width W [--nodes 5]
///     build/methods/bilinear_rank/branch_and_bound/tighten-rank-bound FIXTURE --width W [--nodes 5] --general-span
///
/// | question | slice | nodes | children | general | GF(2) | factor |
/// |---|---|---|---|---|---|---|
/// | `matmul_2x2x2 --width 4` | 4x4 | 21 | 2 251 | 0.05 s | 0.02 s | 2.5x |
/// | `matmul_2x2x2 --width 8` | 4x4 | 40 | 4 359 | 0.09 s | 0.03 s | 3.0x |
/// | `gf64_multiplication --width 4 --nodes 5` * | 6x6 | 10 | 19 780 | 112.95 s | 12.74 s | **8.9x** |
/// | `gf64_multiplication --width 8 --nodes 5` | 6x6 | 10 | 19 780 | 121.39 s | 13.69 s | **8.9x** |
/// | `cyclic_f2_7 --width 4` * | 7x7 | 22 | 17 371 | 12.15 s | 0.64 s | **19.0x** |
/// | `cyclic_f2_7 --width 8` * | 7x7 | 74 | 57 958 | 39.74 s | 2.09 s | **19.0x** |
/// | `<3,4,5> --width 4 --nodes 1 --summand-rank 4` | 12x20 | 1 | 26 040 | 233.93 s | 24.31 s | **9.6x** |
///
/// The last row is the question that asked for this file and ships no fixture:
/// `make-tensor --matmul 2 3 4 5` writes it. One node of it, twice, is what a
/// comparison can afford, and `--nodes 5` on `gf64` is the same kind of price:
/// the whole run there is unaffordable on the general path at any width, which
/// is the point rather than a limit on the finding.
///
/// **Every count is the same count on both paths**: nodes, children, moves
/// offered, improvements, branches bounded, depth, and the algorithm at the end
/// of it. That is what says these are two clocks on one tree rather than two
/// searches. The two `gf64` rows are the same ten nodes because `--nodes 5`
/// binds before the width does; only `branches bounded` separates them, 12
/// against 24.
///
/// **2.5x to 19.0x, and the spread is the point rather than the top of it.**
/// The factor is what the span walk was as a share of the run, times what the
/// arithmetic gained, and neither end of the table is the arithmetic:
///
///  - **`matmul_2x2x2` is small, and not because of start-up.** `--nodes 0` on
///    it is under the 10 ms clock on both paths, so the fixed cost is not what
///    dilutes the row: at dimension 4 to 7 the span walk is simply not most of
///    what that search does, and generating and filtering moves is.
///  - **The widest matrix does not win, which a storage argument gets
///    backwards.** `<3,4,5>` packs a 12x20 slice into four words, the largest
///    saving per matrix in the table, and gains 9.6x where 7x7 gains 19.2x;
///    `gf64`'s 6x6 gains 8.1x with a tree that goes deeper than
///    `cyclic_f2_7`'s. What the two low rows share is the dimension: a call
///    walks 2^16 and 2^17 slots there against 2^14 on `cyclic_f2_7`. The rank
///    and the walk step are what got cheap; the index arithmetic, the floor's
///    lookup, the candidate list and its sort are what did not, and those grow
///    as `p^dim` while the ranks shrink under them. **That is a reading of the
///    rows and not a profile**: `perf_event_paranoid` is 4 on this machine, so
///    nothing here has been profiled and the reading is owed a check.
///
/// **The three starred rows were re-taken on 2026-08-23 to protocol**, at load
/// 0.98 and a package temperature of 43 C, fastest of three, each attempt under
/// `flock /tmp/bilinear-measure.lock`. Those seconds are measurements.
///
/// **The unstarred rows are not.** They were taken on 2026-08-22 at load 2.5 to
/// 5.0, which [`../../../MEASURING.md`](../../../MEASURING.md) abandons above 1.0: another
/// session held two of the twelve cores throughout and waiting it out was not
/// available then. The three attempts of the two paths were **interleaved**
/// rather than run in two blocks, so the neighbour is shared between the columns
/// instead of landing on one, and the spread between attempts is under 7% on
/// every row. Those seconds are upper bounds on a quiet machine's.
///
/// **What the re-take settles is that the ratios were never the doubtful part.**
/// The seconds fell by 7 to 9% once the machine was quiet and every ratio held or
/// improved: 19.2x became 19.0x, 19.0x stayed, and `gf64`'s 8.1x became 8.9x,
/// which is its own width-8 figure. A loaded machine was slowing both columns by
/// about the same amount, which is what interleaving the attempts was for.
/// re-taking under the protocol before it is quoted as one.
///
/// **It is a case chosen at run time, not a build.** The characteristic comes
/// off the tensor file, so `gf2_span_walk_applies` is asked once per call and
/// the general path is what answers when it says no. Nothing over GF(3), GF(5)
/// or the rationals reaches this file, and
/// [`tests/test_gf2_span_walk.cpp`](tests/test_gf2_span_walk.cpp) is what holds
/// the two paths to one answer.
///
/// **Nothing here is a second bit representation.**
/// [`../../../core/linear_algebra/gf2_bits.h`](../../../core/linear_algebra/gf2_bits.h) already had
/// the packing, the exclusive or and the row reader, and
/// [`../../../core/linear_algebra/gf2_span_basis.h`](../../../core/linear_algebra/gf2_span_basis.h)
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
/// The width limit is [`gf2_row`](../../../core/linear_algebra/gf2_bits.h)'s, which
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
/// `tighten-rank-bound` sets this false for the run, the general path answers
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
