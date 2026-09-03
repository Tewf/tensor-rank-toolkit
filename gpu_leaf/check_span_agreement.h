#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "gf2_bits.h"
#include "measures.h"
#include "minimum_weight_basis.h"
#include "span_ranks.h"

/// Whether the card and `span_element_ranks` ranked the same span the same way,
/// **element for element**.
///
/// The argument that they must is shorter than the leaf's, and it is worth saying
/// why. A leaf hands its caller a rank-one basis built by a greedy, which is a
/// loop-carried dependency, so
/// [`why-the-answer-is-the-same.md`](why-the-answer-is-the-same.md) has to
/// separate the filter from the greedy and put the order back with a sort. There
/// is no greedy here. `span_element_ranks` returns one number per index, the
/// number depends on the element alone, and the element depends on the index
/// alone, so the order elements are visited in is invisible to the answer, which
/// is the same fact that let the host walk them in Gray order. A kernel is that
/// same independence taken to 2 048 threads.
///
/// **An argument is not a test**, and the arithmetic is not the same arithmetic:
/// the host runs Gaussian elimination through Givaro over `int64_t` and the
/// kernel does exclusive-or on bit-packed rows. Two implementations of "rank over
/// GF(2)" agreeing is exactly what has to be checked rather than assumed, so
/// every slot is compared and the count of disagreements is what is reported.
/// Equal sizes would not be evidence and are never what is compared.
namespace gpu_leaf {

/// The span as the kernel reads it, from the slices the search holds.
///
/// The packing is [`gf2_pack`](../linear_algebra/gf2_bits.h)'s and the slice
/// order is the caller's, which is what makes index `i` here the same element as
/// index `i` there.
inline SpanQuestion packed_span(const std::vector<bilinear_rank::Matrix>& slices) {
    SpanQuestion question;
    if (slices.empty()) return question;
    question.rows = slices.front().rows();
    question.columns = slices.front().columns();
    question.words = linear_algebra::gf2_word_count(question.rows * question.columns);
    question.slices = slices.size();
    question.slice_rows.resize(question.slices * question.words);
    for (std::size_t slice = 0; slice < slices.size(); ++slice) {
        linear_algebra::gf2_pack(slices[slice].data(), question.rows * question.columns,
                                 &question.slice_rows[slice * question.words]);
    }
    return question;
}

/// How many of the `elements` ranks the two disagree about. Zero is the only
/// acceptable answer.
inline std::size_t ranks_that_differ(const std::vector<std::size_t>& host,
                                     const std::vector<std::uint8_t>& card) {
    if (host.size() != card.size()) return host.size() + card.size();
    std::size_t differ = 0;
    for (std::size_t index = 0; index < host.size(); ++index) {
        differ += host[index] == static_cast<std::size_t>(card[index]) ? 0 : 1;
    }
    return differ;
}

/// A span of `slices` slices of the given shape over GF(2), scattered so that the
/// ranks in it are not all the same number.
///
/// A comparison over a span whose every element has one rank compares almost
/// nothing, which is the same trap `dense_span` exists to avoid on the leaf side.
inline std::vector<bilinear_rank::Matrix> scattered_slices(std::size_t rows, std::size_t columns,
                                                           std::size_t slices, std::uint64_t seed) {
    std::vector<bilinear_rank::Matrix> made;
    for (std::size_t slice = 0; slice < slices; ++slice) {
        bilinear_rank::Matrix map(rows, columns);
        for (std::size_t entry = 0; entry < map.entry_count(); ++entry) {
            seed += 0x9E3779B97F4A7C15ull;
            std::uint64_t mixed = (seed ^ (seed >> 30)) * 0xBF58476D1CE4E5B9ull;
            mixed = (mixed ^ (mixed >> 27)) * 0x94D049BB133111EBull;
            map.data()[entry] = static_cast<bilinear_rank::Element>((mixed >> 31) & 1u);
        }
        made.push_back(std::move(map));
    }
    return made;
}

}  // namespace gpu_leaf
