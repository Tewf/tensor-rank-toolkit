#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "card_failure.h"

/// How a card offers to rank every element of a span, and how
/// [`span_element_ranks`](minimum_weight_basis.h) asks it to.
///
/// **It is the right shape for a card.** `p^dim` independent rank computations
/// over GF(2), each on a small matrix a thread rebuilds from its own index:
/// nothing is transferred per element, every thread reads the same few kilobytes
/// of slices, and the arithmetic is branch-free. That is the same shape as
/// [`../../../infrastructure/gpu_leaf/pool_scan.cu`](../../../infrastructure/gpu_leaf/pool_scan.cu).
///
/// **What holds it back is the shape list and not the launch floor**, and this
/// file said the opposite until 2026-08-22. The spans are well past the floor:
/// `tighten-rank-bound` on `<3,4,5>` prints *"start: 60 products over 15
/// dimensions"*, and a node's dimension only climbs, so every span it ranks is
/// `2^15 = 32 768` elements against the 8 192 in
/// [`../../../infrastructure/run_limits/device.h`](../../../infrastructure/run_limits/device.h); `gf32_multiplication`
/// reaches dimension 13, which is 8 192 exactly and which
/// [`../branch_and_bound/cost_first_search.cpp`](../branch_and_bound/cost_first_search.cpp)
/// already records. `chosen_device` passes both.
///
/// What declines them is `handles`. `span_ranks_handle` in
/// [`../../../infrastructure/gpu_leaf/span_ranks.cu`](../../../infrastructure/gpu_leaf/span_ranks.cu) carries four
/// **square** shapes, 4x4, 5x5, 9x9 and 16x16, because those are the shapes the
/// leaf kernels were instantiated at. `<3,4,5>`'s operands are 12x20, so it is
/// refused on shape while being four times the size the floor asks for. Adding it
/// is a `rank_kernel<12, 20>` instantiation and a case in `launch`, not an
/// install: the template already holds it, since 20 columns is under the 32 a row
/// is carried in and 240 bits is under the four words `kWordCeiling` allows.
///
/// **So this is a seam a card would already use, on the shapes it has.** A build
/// with `nvcc` sends `gf32_multiplication`'s dimension-13 nodes to the card
/// today. The claim that it never fires was wrong, and it was wrong because
/// nothing could test it: [`tests/test_span_ranks_seam.cpp`](tests/test_span_ranks_seam.cpp)
/// now exercises all five gates with a fake backend and no toolkit at all.
///
/// The seam is here and not in [`../../../infrastructure/gpu_leaf/`](../../../infrastructure/gpu_leaf/) for the
/// reason [`../exhaustive/gf2_leaf_on_card.h`](../exhaustive/gf2_leaf_on_card.h)'s
/// is where it is: a build without `nvcc` must get "no" rather than a link
/// error, so the question is asked of a registration that nothing registered.
///
/// **Declining is not failing.** Another field, a shape with no kernel, a span
/// under the launch floor: all of those are the host's job and none of them is
/// news. A CUDA call that failed is news, and goes to
/// [`card_failure.h`](../../../infrastructure/run_limits/card_failure.h) so a command can say once that the run got
/// slower and not that the answer changed.
namespace bilinear_rank {

/// A span as a backend reads it: the shape, and the slices packed one bit per
/// entry.
///
/// `slice_rows` is `slices` rows of `words` words, **in the order the slices were
/// given**, because that order *is* the index: element `i` is the exclusive or of
/// the slices whose bit is set in `i`, which is what
/// [`coefficient_vector`](span_enumeration.h) means over GF(2). A backend may not
/// reorder them.
struct PackedSpan {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t words = 0;
    std::size_t slices = 0;

    const std::uint64_t* slice_rows = nullptr;
};

/// What a card offers: every rank or none of them.
struct SpanRanksOnCard {
    /// Whether a kernel is compiled for this shape.
    bool (*handles)(std::size_t rows, std::size_t columns);

    /// `ranks[i]` for every `i` in `[0, elements)`, the rank of span element `i`.
    ///
    /// **Whole, in place, or declined.** `ranks` is sized by the caller and is
    /// written entry for entry; a backend that cannot answer the whole range
    /// returns false and leaves it untouched, because a partly filled vector of
    /// ranks is not a short answer, it is a wrong one that no caller can see.
    bool (*ranks)(const PackedSpan& span, std::uint64_t elements,
                  std::vector<std::size_t>& ranks);
};

/// Registered once, by whoever links a backend. Nothing registers one on a
/// machine without `nvcc`, and `span_ranks_on_card()` is then null for the whole
/// run.
void register_span_ranks_on_card(const SpanRanksOnCard* backend);
const SpanRanksOnCard* span_ranks_on_card();

}  // namespace bilinear_rank
