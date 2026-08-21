#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "card_failure.h"

/// How a card offers to rank every element of a span, and how
/// [`span_element_ranks`](minimum_weight_basis.h) asks it to.
///
/// **It is the right shape for a card and it does not pay on this machine, and
/// both of those are the point.** `p^dim` independent rank computations over
/// GF(2), each on a small matrix a thread rebuilds from its own index: nothing is
/// transferred per element, every thread reads the same few kilobytes of slices,
/// and the arithmetic is branch-free. That is the same shape as
/// [`../gpu_leaf/pool_scan.cu`](../gpu_leaf/pool_scan.cu). What differs is the
/// size: the spans this search reaches are 2 048 to 8 192 elements, against a
/// measured launch floor of 8 192 in
/// [`../run_limits/device.h`](../run_limits/device.h), so `chosen_device` keeps
/// almost all of them on the host — and that is the correct answer rather than a
/// disappointment. The seam exists so that hardware where the same question is
/// larger has somewhere to put it.
///
/// The seam is here and not in [`../gpu_leaf/`](../gpu_leaf/README.md) for the
/// reason [`../exhaustive_search/gf2_leaf_on_card.h`](../exhaustive_search/gf2_leaf_on_card.h)'s
/// is where it is: a build without `nvcc` must get "no" rather than a link
/// error, so the question is asked of a registration that nothing registered.
///
/// **Declining is not failing.** Another field, a shape with no kernel, a span
/// under the launch floor: all of those are the host's job and none of them is
/// news. A CUDA call that failed is news, and goes to
/// [`card_failure.h`](card_failure.h) so a command can say once that the run got
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
