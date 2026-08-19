#pragma once

#include <cstdint>
#include <vector>

#include "gf2_bits.h"
#include "leaf_question.h"

/// The two leaf routes on one host core, in the kernel's representation,
/// reporting every survivor by index.
///
/// It is here for two reasons and both of them are measurement. It is the
/// element-for-element reference the kernel's survivor list is compared against,
/// which a `Gf2Leaf` cannot be because that one hands back the independent maps
/// it kept rather than everything that passed. And it separates the card from
/// the representation: the leaf this repository ships derives its candidate
/// through [`RankOnePool::at`](../descent_search/candidate_pool.h), which is 256
/// Givaro multiplications and a heap allocation, where a kernel derives it with
/// two shifts. Timing the kernel against the shipped leaf alone would credit the
/// GPU with a win that is partly the bit trick, so both are measured.
///
/// The reductions and the rank-one test are
/// [`gf2_bits.h`](../linear_algebra/gf2_bits.h)'s, unchanged, so nothing about
/// the answer is restated here.
namespace gpu_leaf {

/// Pool element `left ⊗ right`, packed, without building a matrix.
///
/// Row `i` of the outer product is the right vector when the left vector's bit
/// `i` is set and zero when it is not, so the whole element is the right mask
/// shifted into place once per set bit of the left mask. This is the arithmetic
/// the kernel does, and it is what a packed `at(i)` would be on either machine.
inline void packed_outer_product(const LeafQuestion& question, std::uint32_t left,
                                 std::uint32_t right, std::uint64_t* out) {
    for (std::size_t word = 0; word < question.words; ++word) out[word] = 0;
    for (std::size_t row = 0; row < question.rows; ++row) {
        if (((left >> row) & 1u) == 0) continue;
        const std::size_t start = row * question.columns;
        const std::size_t word = start / 64, offset = start % 64;
        out[word] |= std::uint64_t(right) << offset;
        if (offset + question.columns > 64) {
            out[word + 1] |= std::uint64_t(right) >> (64 - offset);
        }
    }
}

/// Whether a packed candidate lies in the span: the reduction
/// [`Gf2SpanBasis`](../linear_algebra/gf2_span_basis.h) performs, written
/// without its branch.
///
/// **The branch is the reason this is not simply a call into that class.** Which
/// basis rows apply is a property of the candidate, and over four billion
/// candidates each pivot is hit about half the time in no pattern, so a
/// conditional exclusive or here is forty-seven unpredictable branches per
/// element. Masking instead of branching is what a kernel is forced to do and is
/// the faster thing to do on a core as well, so the host is given it too:
/// crediting the card with the host's mispredictions would not be a measurement
/// of the card.
///
/// The arithmetic is unchanged, so a candidate reduces to zero here exactly when
/// it does there, which is what
/// [`check_agreement.h`](check_agreement.h) asserts on every shape.
inline bool in_the_span(const LeafQuestion& question, const std::uint64_t* candidate,
                        std::uint64_t* scratch) {
    for (std::size_t word = 0; word < question.words; ++word) scratch[word] = candidate[word];
    for (std::size_t row = 0; row < question.dimension(); ++row) {
        const std::uint32_t pivot = question.pivots[row];
        const std::uint64_t hit = (scratch[pivot / 64] >> (pivot % 64)) & 1;
        const std::uint64_t select = ~(hit - 1);
        const std::uint64_t* source = &question.span_rows[row * question.words];
        for (std::size_t word = 0; word < question.words; ++word) {
            scratch[word] ^= select & source[word];
        }
    }
    return linear_algebra::gf2_is_zero(scratch, question.words);
}

/// Pool elements in `[left_begin, left_end) x every right` that lie in the span,
/// by index.
///
/// The range is whole rows of the outer-product grid, which is how the kernel
/// splits its launches, so a range here and a launch there cover the same
/// elements.
inline std::vector<std::uint64_t> scan_pool_on_host(const LeafQuestion& question,
                                                    std::size_t left_begin, std::size_t left_end) {
    std::vector<std::uint64_t> survivors;
    std::vector<std::uint64_t> candidate(question.words), scratch(question.words);
    for (std::size_t left = left_begin; left < left_end; ++left) {
        for (std::size_t right = 0; right < question.right_count; ++right) {
            packed_outer_product(question, question.left_masks[left], question.right_masks[right],
                                 candidate.data());
            if (!in_the_span(question, candidate.data(), scratch.data())) continue;
            survivors.push_back(left * question.right_count + right);
        }
    }
    return survivors;
}

/// The subspace element at `index`: the exclusive or of the span rows whose bit
/// is set in the index, which is the digit expansion the general path multiplies
/// by.
inline void subspace_element(const LeafQuestion& question, std::uint64_t index,
                             std::uint64_t* out) {
    for (std::size_t word = 0; word < question.words; ++word) out[word] = 0;
    for (std::size_t row = 0; row < question.dimension(); ++row) {
        if (((index >> row) & 1) == 0) continue;
        linear_algebra::gf2_xor(out, &question.span_rows[row * question.words], question.words);
    }
}

/// Subspace elements in `[begin, end)` that have rank one, by index.
inline std::vector<std::uint64_t> walk_subspace_on_host(const LeafQuestion& question,
                                                        std::uint64_t begin, std::uint64_t end) {
    std::vector<std::uint64_t> survivors;
    std::vector<std::uint64_t> element(question.words);
    for (std::uint64_t index = begin; index < end; ++index) {
        subspace_element(question, index, element.data());
        if (linear_algebra::gf2_is_rank_one(element.data(), question.rows, question.columns)) {
            survivors.push_back(index);
        }
    }
    return survivors;
}

}  // namespace gpu_leaf
