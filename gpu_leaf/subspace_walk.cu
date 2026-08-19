#include <algorithm>
#include <chrono>

#include "cuda_guard.cuh"
#include "gpu_leaf.h"

/// The subspace walk on the card: one thread per subspace element, tested for
/// rank one.
///
/// This is the other of the two routes [`rank_one_basis.h`](../exhaustive_search/rank_one_basis.h)
/// chooses between, and the one the bit-packed leaf gained most on, 39.6x where
/// the pool scan gained 6x to 23x. It is also the only route the shapes with
/// known answers take, so leaving it out would mean the card never runs where an
/// answer can be checked against one.
namespace gpu_leaf {
namespace {

constexpr int kWordCeiling = 4;
/// A walk enumerates `2^dimension` elements, so a dimension a walk can finish is
/// far below this; it is here to bound the constant array, not the question.
constexpr int kDimensionCeiling = 64;

__constant__ std::uint64_t c_walk_rows[kDimensionCeiling * kWordCeiling];

/// One row of a packed matrix, as the low `COLUMNS` bits of a word. The shape is
/// a template parameter, so the shifts are literals and the element stays in
/// registers.
template <int ROWS, int COLUMNS, int WORDS>
__device__ inline std::uint64_t row_of(const std::uint64_t* element, int row) {
    const int start = row * COLUMNS;
    std::uint64_t value = element[start / 64] >> (start % 64);
    if (start % 64 + COLUMNS > 64) value |= element[start / 64 + 1] << (64 - start % 64);
    return value & ((std::uint64_t(1) << COLUMNS) - 1);
}

/// One thread, one subspace element: exclusive or of the span rows its index
/// selects, then the rank-one test.
///
/// Over GF(2) a nonzero matrix has rank one exactly when its nonzero rows are
/// all the same row, which is [`gf2_is_rank_one`](../linear_algebra/gf2_bits.h).
/// Written here without the early returns that version takes, because a warp
/// pays for every branch any of its threads takes: the rows are or-ed together
/// and then each nonzero row is compared against that, which is the same test
/// with no exit.
template <int ROWS, int COLUMNS>
__global__ void walk_kernel(unsigned long long begin, unsigned long long end, int dimension,
                            unsigned long long* __restrict__ survivors,
                            unsigned int* __restrict__ found, unsigned int capacity,
                            int* __restrict__ overflow) {
    constexpr int kWords = (ROWS * COLUMNS + 63) / 64;
    const unsigned long long index =
        begin + blockIdx.x * static_cast<unsigned long long>(blockDim.x) + threadIdx.x;
    if (index >= end) return;

    std::uint64_t element[kWords];
#pragma unroll
    for (int word = 0; word < kWords; ++word) element[word] = 0;
    for (int row = 0; row < dimension; ++row) {
        const std::uint64_t select = -static_cast<std::uint64_t>((index >> row) & 1ull);
#pragma unroll
        for (int word = 0; word < kWords; ++word) {
            element[word] ^= select & c_walk_rows[row * kWords + word];
        }
    }

    std::uint64_t together = 0;
#pragma unroll
    for (int row = 0; row < ROWS; ++row) together |= row_of<ROWS, COLUMNS, kWords>(element, row);
    std::uint64_t disagreement = 0;
#pragma unroll
    for (int row = 0; row < ROWS; ++row) {
        const std::uint64_t entries = row_of<ROWS, COLUMNS, kWords>(element, row);
        disagreement |= entries != 0 ? (entries ^ together) : 0ull;
    }
    if (together == 0 || disagreement != 0) return;

    const unsigned int slot = atomicAdd(found, 1u);
    if (slot < capacity) {
        survivors[slot] = index;
    } else {
        *overflow = 1;
    }
}

struct DeviceBuffer {
    void* pointer = nullptr;
    explicit DeviceBuffer(std::size_t bytes) { GPU_LEAF_CHECK(cudaMalloc(&pointer, bytes)); }
    ~DeviceBuffer() { cudaFree(pointer); }
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
};

void launch(const LeafQuestion& question, unsigned int blocks, unsigned int threads,
            unsigned long long begin, unsigned long long end, unsigned long long* survivors,
            unsigned int* found, unsigned int capacity, int* overflow) {
    const int dimension = static_cast<int>(question.dimension());
    const std::size_t shape = question.rows * 100 + question.columns;
    switch (shape) {
        case 404:
            walk_kernel<4, 4><<<blocks, threads>>>(begin, end, dimension, survivors, found,
                                                   capacity, overflow);
            break;
        case 505:
            walk_kernel<5, 5><<<blocks, threads>>>(begin, end, dimension, survivors, found,
                                                   capacity, overflow);
            break;
        case 909:
            walk_kernel<9, 9><<<blocks, threads>>>(begin, end, dimension, survivors, found,
                                                   capacity, overflow);
            break;
        case 1616:
            walk_kernel<16, 16><<<blocks, threads>>>(begin, end, dimension, survivors, found,
                                                     capacity, overflow);
            break;
        default:
            throw std::runtime_error("no kernel compiled for this shape");
    }
    GPU_LEAF_CHECK(cudaGetLastError());
}

}  // namespace

GpuSurvivors walk_subspace_on_gpu(const LeafQuestion& question, std::uint64_t begin,
                                  std::uint64_t end, std::size_t capacity) {
    constexpr unsigned int kThreadsPerBlock = 256;
    constexpr unsigned long long kElementsPerLaunch = 1ull << 26;
    if (question.dimension() > kDimensionCeiling) {
        throw std::runtime_error("span wider than the kernel holds");
    }

    DeviceBuffer survivors(capacity * sizeof(unsigned long long));
    DeviceBuffer counters(2 * sizeof(unsigned int));

    cudaEvent_t opened, closed;
    GPU_LEAF_CHECK(cudaEventCreate(&opened));
    GPU_LEAF_CHECK(cudaEventCreate(&closed));

    const auto started = std::chrono::steady_clock::now();
    GPU_LEAF_CHECK(cudaMemcpyToSymbol(c_walk_rows, question.span_rows.data(),
                                      question.span_rows.size() * sizeof(std::uint64_t)));
    GPU_LEAF_CHECK(cudaMemset(counters.pointer, 0, 2 * sizeof(unsigned int)));

    GpuSurvivors result;
    for (std::uint64_t first = begin; first < end; first += kElementsPerLaunch) {
        const std::uint64_t last = std::min<std::uint64_t>(first + kElementsPerLaunch, end);
        const unsigned int blocks =
            static_cast<unsigned int>((last - first + kThreadsPerBlock - 1) / kThreadsPerBlock);
        GPU_LEAF_CHECK(cudaEventRecord(opened));
        launch(question, blocks, kThreadsPerBlock, first, last,
               static_cast<unsigned long long*>(survivors.pointer),
               static_cast<unsigned int*>(counters.pointer), static_cast<unsigned int>(capacity),
               static_cast<int*>(counters.pointer) + 1);
        GPU_LEAF_CHECK(cudaEventRecord(closed));
        GPU_LEAF_CHECK(cudaEventSynchronize(closed));
        float milliseconds = 0.0f;
        GPU_LEAF_CHECK(cudaEventElapsedTime(&milliseconds, opened, closed));
        result.kernel_seconds += milliseconds / 1000.0;
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
    result.wall_seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();

    cudaEventDestroy(opened);
    cudaEventDestroy(closed);
    return result;
}

}  // namespace gpu_leaf
