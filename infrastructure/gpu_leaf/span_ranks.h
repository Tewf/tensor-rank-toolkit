#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/// Ranking every element of a GF(2) span on the card, as one call a host program
/// can make.
///
/// Declared away from the `.cu` file so that
/// [`span_ranks_backend.cpp`](span_ranks_backend.cpp) and
/// [`measure_leaf.cpp`](measure_leaf.cpp) are ordinary C++ translation units and
/// only the kernel goes through `nvcc`, exactly as [`gpu_leaf.h`](gpu_leaf.h)
/// does for the two leaf routes.
///
/// The question this answers is
/// [`span_element_ranks`](../../methods/bilinear_rank/greedy_heuristic/minimum_weight_basis.h)'s,
/// and it has the shape the leaf kernels have: `2^slices` independent
/// computations, each on a small matrix a thread rebuilds from its own index by
/// exclusive-or, nothing transferred per element. What it does not have is their
/// size, which is why
/// [`span_ranks_on_card.h`](../../methods/bilinear_rank/greedy_heuristic/span_ranks_on_card.h)
/// says the host will usually and correctly answer instead.
namespace gpu_leaf {

/// A span in the representation a kernel reads: plain data, and no Givaro.
///
/// `slice_rows` is `slices` rows of `words` words, in the order the slices were
/// given. **That order is the index**: element `i` is the exclusive or of the
/// slices whose bit is set in `i`. A reordering here is not a different order of
/// visiting, it is a different answer in every slot.
struct SpanQuestion {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t words = 0;
    std::size_t slices = 0;
    std::vector<std::uint64_t> slice_rows;

    std::uint64_t elements() const { return std::uint64_t(1) << slices; }
};

/// What one run of the kernel produced, and what it cost.
struct GpuRanks {
    /// One rank per element of `[begin, end)`, in index order. A rank of a small
    /// GF(2) matrix does not reach 256, and the widest shape compiled here is
    /// 16x16, so a byte holds it and the copy back is one byte an element.
    std::vector<std::uint8_t> ranks;
    /// Seconds inside the kernels, from CUDA events, summed over the launches.
    double kernel_seconds = 0.0;
    /// Seconds from before the first launch to after the last copy back, which
    /// is what the work actually cost the host that asked for it.
    double wall_seconds = 0.0;
};

/// The rank of span element `i` for every `i` in `[begin, end)`.
GpuRanks rank_span_on_gpu(const SpanQuestion& question, std::uint64_t begin, std::uint64_t end);

/// Whether a kernel is compiled for this shape. The same four the leaf kernels
/// carry, because they are the ones the fixtures here reach.
bool span_ranks_handle(std::size_t rows, std::size_t columns);

}  // namespace gpu_leaf
