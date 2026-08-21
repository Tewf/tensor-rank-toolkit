#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "card_failure.h"

/// How a card offers to answer a GF(2) leaf, and how the search asks it to.
///
/// The seam is here rather than in [`gpu_leaf/`](../gpu_leaf/gpu_leaf.h) for the
/// reason [`../run_limits/device.h`](../run_limits/device.h)'s is in
/// `run_limits`: the search must be able to ask on a machine that has no card
/// and a build that has no kernels, and get "no" rather than a link error. A
/// build with `nvcc` registers a backend and a build without one registers
/// nothing, and neither is a special case anywhere else.
///
/// **Every entry may decline, and declining is not a failure.** A shape with no
/// kernel, a span wider than a kernel holds, a leaf under the launch floor: all
/// of those are the host's job and none of them is news. What *is* news is a
/// CUDA call that failed, which is recorded by
/// [`note_card_failure`](../descent_search/card_failure.h) and printed once by
/// the command, because a card that silently stopped being used would otherwise
/// show up as a run that got mysteriously slower. That record is shared with
/// every seam that offers work to a card, because there is one card.
///
/// **A backend never decides whether it should run.** `Gf2Leaf` asks
/// `run_limits::chosen_device` first and bounds the range by `may_examine`
/// before it calls in, so the launch floor and the leaf budget are enforced in
/// the one place that enforces them for the host too.
namespace bilinear_rank {

/// One leaf as a backend reads it: a borrowed view of what `Gf2Leaf` already
/// holds, in the packed form both sides work in. Plain pointers, because the
/// caller owns every one of these arrays for the length of the call and copying
/// a 16 MB mask table per leaf would be its own measurement.
///
/// `span_rows` is `dimension` rows of `words` words **in the order the span
/// hands them over**, which is the order the subspace walk reads its digits in.
/// A backend may reorder them for a reduction, where the order cannot matter
/// because the rows are in reduced row echelon form, and may not reorder them
/// for a walk, where the order *is* the index.
struct PackedLeaf {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t words = 0;
    std::size_t left_count = 0;
    std::size_t right_count = 0;
    std::size_t dimension = 0;

    const std::uint64_t* left_masks = nullptr;
    const std::uint64_t* right_masks = nullptr;
    const std::uint64_t* span_rows = nullptr;
    const std::uint32_t* pivots = nullptr;
};

/// What a card offers. Each function answers whole and in order, or declines.
///
/// **`survivors` comes back ascending or not at all.** The greedy the host runs
/// over it is order-dependent, so ascending index order is what makes the maps
/// it keeps the same maps the sequential loop kept — see
/// [`../gpu_leaf/why-the-answer-is-the-same.md`](../gpu_leaf/why-the-answer-is-the-same.md).
/// A backend that cannot report every survivor of the range returns false and
/// leaves `survivors` untouched, because a truncated list is not a short answer,
/// it is a wrong one.
struct LeafOnCard {
    /// Whether a kernel is compiled for this shape.
    bool (*handles)(std::size_t rows, std::size_t columns);

    /// Pool indices in `[0, left_rows * right_count)` that lie in the span.
    /// Whole rows of the outer-product grid, which is the smallest unit either
    /// side works in.
    bool (*scan)(const PackedLeaf& leaf, std::size_t left_rows,
                 std::vector<std::uint64_t>& survivors);

    /// Subspace indices in `[1, elements)` that have rank one. Index zero is the
    /// zero map, which the host's walk does not examine either.
    bool (*walk)(const PackedLeaf& leaf, std::uint64_t elements,
                 std::vector<std::uint64_t>& survivors);
};

/// Registered once, by whoever links a backend. Nothing registers one on a
/// machine without `nvcc`, and `leaf_on_card()` is then null for the whole run.
void register_leaf_on_card(const LeafOnCard* backend);
const LeafOnCard* leaf_on_card();

}  // namespace bilinear_rank
