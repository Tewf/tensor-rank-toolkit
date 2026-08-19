#include <algorithm>
#include <chrono>
#include <numeric>

#include "cuda_guard.cuh"
#include "gpu_leaf.h"

/// The pool scan on the card: one thread per rank-one map, nothing transferred.
///
/// The pool is the outer-product grid of two vector lists, so element
/// `left * right_count + right` is a pure function of its index and a thread
/// derives its own. At the 16x16 slices of `⟨4,4,4⟩` that is 4 294 836 225 maps
/// which are never stored anywhere, on either machine: what the card holds is
/// the two lists at 262 KB together, the span in constant memory at 2 KB, and a
/// survivor buffer. [`README.md`](README.md) says why that is the whole reason
/// this is worth trying.
namespace gpu_leaf {
namespace {

/// 256 bits is the widest map this repository prices, the 16x16 slices of
/// `⟨4,4,4⟩`, and a dimension cannot exceed the width.
constexpr int kWordCeiling = 4;
constexpr int kDimensionCeiling = 256;

__constant__ std::uint64_t c_span_rows[kDimensionCeiling * kWordCeiling];
__constant__ std::uint64_t c_pivot_mask[kDimensionCeiling];
__constant__ int c_segment_end[kWordCeiling];

/// One thread, one pool element: derive it, reduce it against the span, and
/// report the index if nothing is left.
///
/// The shape is a template parameter so the bit positions of the rows are
/// literals after unrolling and the candidate stays in registers; a runtime
/// shape would index a local array and spill it to memory, which is the one
/// thing this kernel must not do.
///
/// The reduction is branch-free. Whether a basis row applies is a mask and not
/// an `if`, so a warp of 32 candidates costs what one costs, which is the
/// property the tree above this leaf does not have.
template <int ROWS, int COLUMNS>
__global__ void scan_kernel(const std::uint32_t* __restrict__ left_masks,
                            const std::uint32_t* __restrict__ right_masks,
                            unsigned long long left_begin, unsigned int right_count,
                            unsigned long long* __restrict__ survivors,
                            unsigned int* __restrict__ found, unsigned int capacity,
                            int* __restrict__ overflow) {
    constexpr int kWords = (ROWS * COLUMNS + 63) / 64;
    const unsigned int right = blockIdx.x * blockDim.x + threadIdx.x;
    if (right >= right_count) return;

    const unsigned long long left = left_begin + blockIdx.y;
    const std::uint32_t left_mask = left_masks[left];
    const std::uint64_t right_word = right_masks[right];

    std::uint64_t map[kWords];
#pragma unroll
    for (int word = 0; word < kWords; ++word) map[word] = 0;
#pragma unroll
    for (int row = 0; row < ROWS; ++row) {
        const std::uint64_t value = ((left_mask >> row) & 1u) != 0u ? right_word : 0ull;
        const int start = row * COLUMNS;
        map[start / 64] |= value << (start % 64);
        if (start % 64 + COLUMNS > 64) map[start / 64 + 1] |= value >> (64 - start % 64);
    }

    int row = 0;
#pragma unroll
    for (int word = 0; word < kWords; ++word) {
        const int end = c_segment_end[word];
        for (; row < end; ++row) {
            const std::uint64_t select = (map[word] & c_pivot_mask[row]) != 0 ? ~0ull : 0ull;
#pragma unroll
            for (int target = 0; target < kWords; ++target) {
                map[target] ^= select & c_span_rows[row * kWords + target];
            }
        }
    }

    std::uint64_t remaining = 0;
#pragma unroll
    for (int word = 0; word < kWords; ++word) remaining |= map[word];
    if (remaining != 0) return;

    const unsigned int slot = atomicAdd(found, 1u);
    if (slot < capacity) {
        survivors[slot] = left * right_count + right;
    } else {
        *overflow = 1;
    }
}

/// The span in constant memory, its rows grouped by which word their pivot is
/// in, which is what lets the kernel index the candidate statically.
///
/// Regrouping them changes no answer. The rows arrive in reduced row echelon
/// form, so no row carries another's pivot bit, so reducing against one never
/// changes whether another applies and the reduction is the same in any order.
void upload_span(const LeafQuestion& question) {
    const std::size_t words = question.words, dimension = question.dimension();
    if (dimension > kDimensionCeiling) throw std::runtime_error("span wider than the kernel holds");

    std::vector<std::size_t> order(dimension);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
        return question.pivots[left] / 64 < question.pivots[right] / 64;
    });

    std::vector<std::uint64_t> rows(dimension * words), masks(dimension);
    std::vector<int> segment_end(words, 0);
    for (std::size_t position = 0; position < dimension; ++position) {
        const std::size_t source = order[position];
        for (std::size_t word = 0; word < words; ++word) {
            rows[position * words + word] = question.span_rows[source * words + word];
        }
        masks[position] = std::uint64_t(1) << (question.pivots[source] % 64);
        for (std::size_t word = question.pivots[source] / 64; word < words; ++word) {
            segment_end[word] = static_cast<int>(position + 1);
        }
    }
    GPU_LEAF_CHECK(cudaMemcpyToSymbol(c_span_rows, rows.data(), rows.size() * sizeof(std::uint64_t)));
    GPU_LEAF_CHECK(cudaMemcpyToSymbol(c_pivot_mask, masks.data(), masks.size() * sizeof(std::uint64_t)));
    GPU_LEAF_CHECK(cudaMemcpyToSymbol(c_segment_end, segment_end.data(), words * sizeof(int)));
}

/// Launch the shape this question has, or say that this proof of concept does
/// not carry it. The four are the shapes the fixtures here reach.
void launch(const LeafQuestion& question, dim3 grid, unsigned int threads,
            unsigned long long left_begin, const std::uint32_t* lefts,
            const std::uint32_t* rights, unsigned long long* survivors, unsigned int* found,
            unsigned int capacity, int* overflow) {
    const unsigned int right_count = static_cast<unsigned int>(question.right_count);
    const std::size_t shape = question.rows * 100 + question.columns;
    switch (shape) {
        case 404:
            scan_kernel<4, 4><<<grid, threads>>>(lefts, rights, left_begin, right_count, survivors,
                                                 found, capacity, overflow);
            break;
        case 505:
            scan_kernel<5, 5><<<grid, threads>>>(lefts, rights, left_begin, right_count, survivors,
                                                 found, capacity, overflow);
            break;
        case 909:
            scan_kernel<9, 9><<<grid, threads>>>(lefts, rights, left_begin, right_count, survivors,
                                                 found, capacity, overflow);
            break;
        case 1616:
            scan_kernel<16, 16><<<grid, threads>>>(lefts, rights, left_begin, right_count, survivors,
                                                   found, capacity, overflow);
            break;
        default:
            throw std::runtime_error("no kernel compiled for this shape");
    }
    GPU_LEAF_CHECK(cudaGetLastError());
}

}  // namespace

std::string device_description() {
    cudaDeviceProp properties{};
    GPU_LEAF_CHECK(cudaGetDeviceProperties(&properties, 0));
    return std::string(properties.name) + ", compute capability " +
           std::to_string(properties.major) + "." + std::to_string(properties.minor) + ", " +
           std::to_string(properties.multiProcessorCount) + " SMs, " +
           std::to_string(properties.totalGlobalMem / (1024 * 1024)) + " MiB";
}

GpuSurvivors scan_pool_on_gpu(const LeafQuestion& question, std::size_t left_begin,
                              std::size_t left_end, std::size_t capacity) {
    constexpr unsigned int kThreadsPerBlock = 256;
    /// Rows of the grid per launch. One launch of the whole pool would be a
    /// second of kernel and a grid at the 65 535 ceiling of its second
    /// dimension; this keeps both away from the edge.
    constexpr unsigned int kRowsPerLaunch = 2048;
    if (left_end > question.left_count) throw std::runtime_error("range past the end of the grid");

    DeviceBuffer lefts(question.left_masks.size() * sizeof(std::uint32_t));
    DeviceBuffer rights(question.right_masks.size() * sizeof(std::uint32_t));
    DeviceBuffer survivors(capacity * sizeof(unsigned long long));
    DeviceBuffer counters(2 * sizeof(unsigned int));
    GPU_LEAF_CHECK(cudaMemcpy(lefts.pointer, question.left_masks.data(),
                              question.left_masks.size() * sizeof(std::uint32_t),
                              cudaMemcpyHostToDevice));
    GPU_LEAF_CHECK(cudaMemcpy(rights.pointer, question.right_masks.data(),
                              question.right_masks.size() * sizeof(std::uint32_t),
                              cudaMemcpyHostToDevice));

    Event opened, closed;

    // The timer opens here: the two vector lists above are a search's setup and
    // are uploaded once, where the span below changes at every leaf.
    const auto started = std::chrono::steady_clock::now();
    upload_span(question);
    GPU_LEAF_CHECK(cudaMemset(counters.pointer, 0, 2 * sizeof(unsigned int)));

    GpuSurvivors result;
    const unsigned int blocks = (static_cast<unsigned int>(question.right_count) +
                                 kThreadsPerBlock - 1) / kThreadsPerBlock;
    for (std::size_t row = left_begin; row < left_end; row += kRowsPerLaunch) {
        const unsigned int rows =
            static_cast<unsigned int>(std::min<std::size_t>(kRowsPerLaunch, left_end - row));
        GPU_LEAF_CHECK(cudaEventRecord(opened.handle));
        launch(question, dim3(blocks, rows), kThreadsPerBlock, row,
               static_cast<const std::uint32_t*>(lefts.pointer),
               static_cast<const std::uint32_t*>(rights.pointer),
               static_cast<unsigned long long*>(survivors.pointer),
               static_cast<unsigned int*>(counters.pointer), static_cast<unsigned int>(capacity),
               static_cast<int*>(counters.pointer) + 1);
        GPU_LEAF_CHECK(cudaEventRecord(closed.handle));
        GPU_LEAF_CHECK(cudaEventSynchronize(closed.handle));
        result.kernel_seconds += seconds_between(opened, closed);
    }

    unsigned int counted[2] = {0, 0};
    GPU_LEAF_CHECK(cudaMemcpy(counted, counters.pointer, 2 * sizeof(unsigned int),
                              cudaMemcpyDeviceToHost));
    result.overflowed = counted[1] != 0;
    result.indices.resize(std::min<std::size_t>(counted[0], capacity));
    if (!result.indices.empty()) {
        GPU_LEAF_CHECK(cudaMemcpy(result.indices.data(), survivors.pointer,
                                  result.indices.size() * sizeof(unsigned long long),
                                  cudaMemcpyDeviceToHost));
    }
    std::sort(result.indices.begin(), result.indices.end());
    result.wall_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

    return result;
}

}  // namespace gpu_leaf
