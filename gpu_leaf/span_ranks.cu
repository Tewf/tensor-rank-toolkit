#include <algorithm>
#include <chrono>

#include "cuda_guard.cuh"
#include "span_ranks.h"

/// Every element of a GF(2) span ranked on the card: one thread per element,
/// nothing transferred per element.
///
/// The element is derived the way [`subspace_walk.cu`](subspace_walk.cu) derives
/// one — the exclusive or of the slices whose bit is set in the index — and then
/// **ranked** rather than tested for rank one. Over GF(2) the rank of a matrix is
/// the size of a basis of its rows under exclusive or, so the whole computation
/// is an insertion of `ROWS` words into a table indexed by leading bit, with no
/// division, no pivot search and no field arithmetic.
///
/// **It is branch-free for the same reason the leaf kernels are.** A warp of 32
/// elements pays for every branch any of its threads takes, and which row becomes
/// a pivot is a property of the element; so whether a row applies is a mask, the
/// count is an addition, and no thread leaves the loop early. The rows are held
/// as 32-bit words rather than 64-bit: no shape compiled here is wider than
/// sixteen columns, so a row fits, and the table halves in registers.
namespace gpu_leaf {
namespace {

/// 256 bits is the widest map this repository prices, the 16x16 slices of
/// `⟨4,4,4⟩`.
constexpr int kWordCeiling = 4;
/// A span of `s` slices has `2^s` elements, so a span anything can enumerate is
/// far below this; it is here to bound the constant array, not the question.
constexpr int kSliceCeiling = 64;

__constant__ std::uint64_t c_slice_rows[kSliceCeiling * kWordCeiling];

/// One row of a packed matrix, as the low `COLUMNS` bits of a word.
///
/// The shape is a template parameter and `row` arrives from an unrolled loop, so
/// every shift here is a literal and the element stays in registers.
template <int COLUMNS, int WORDS>
__device__ inline unsigned int row_bits(const std::uint64_t* element, int row) {
    static_assert(COLUMNS < 32, "a row is carried in a 32-bit word");
    const int start = row * COLUMNS;
    std::uint64_t value = element[start / 64] >> (start % 64);
    if (start % 64 + COLUMNS > 64) value |= element[start / 64 + 1] << (64 - start % 64);
    return static_cast<unsigned int>(value & ((std::uint64_t(1) << COLUMNS) - 1));
}

/// One thread, one span element: exclusive or of the slices its index selects,
/// then the rank of what that is.
///
/// The rank is the number of rows that survive reduction against the rows before
/// them. `basis[b]` holds the reduced row whose highest set bit is `b`, so
/// walking `b` downwards either clears bit `b` of the row being inserted or
/// installs it and stops — and "stops" is `value &= ~take`, which makes every
/// later iteration a no-op without a branch.
template <int ROWS, int COLUMNS>
__global__ void rank_kernel(unsigned long long begin, unsigned long long end, int slices,
                            unsigned char* __restrict__ ranks) {
    constexpr int kWords = (ROWS * COLUMNS + 63) / 64;
    const unsigned long long index =
        begin + blockIdx.x * static_cast<unsigned long long>(blockDim.x) + threadIdx.x;
    if (index >= end) return;

    std::uint64_t element[kWords];
#pragma unroll
    for (int word = 0; word < kWords; ++word) element[word] = 0;
    for (int slice = 0; slice < slices; ++slice) {
        const std::uint64_t select = -static_cast<std::uint64_t>((index >> slice) & 1ull);
#pragma unroll
        for (int word = 0; word < kWords; ++word) {
            element[word] ^= select & c_slice_rows[slice * kWords + word];
        }
    }

    unsigned int basis[COLUMNS];
#pragma unroll
    for (int column = 0; column < COLUMNS; ++column) basis[column] = 0u;

    int found = 0;
#pragma unroll
    for (int row = 0; row < ROWS; ++row) {
        unsigned int value = row_bits<COLUMNS, kWords>(element, row);
#pragma unroll
        for (int bit = COLUMNS - 1; bit >= 0; --bit) {
            const unsigned int has = (value & (1u << bit)) != 0u ? ~0u : 0u;
            const unsigned int occupied = basis[bit] != 0u ? ~0u : 0u;
            value ^= has & occupied & basis[bit];
            const unsigned int take = has & ~occupied;
            basis[bit] |= take & value;
            found += take != 0u ? 1 : 0;
            value &= ~take;
        }
    }
    ranks[index - begin] = static_cast<unsigned char>(found);
}

/// Launch the shape this question has, or say that this proof of concept does
/// not carry it. The four are the shapes the fixtures here reach, and the same
/// four the leaf kernels are instantiated at.
void launch(const SpanQuestion& question, unsigned int blocks, unsigned int threads,
            unsigned long long begin, unsigned long long end, unsigned char* ranks) {
    const int slices = static_cast<int>(question.slices);
    const std::size_t shape = question.rows * 100 + question.columns;
    switch (shape) {
        case 404:
            rank_kernel<4, 4><<<blocks, threads>>>(begin, end, slices, ranks);
            break;
        case 505:
            rank_kernel<5, 5><<<blocks, threads>>>(begin, end, slices, ranks);
            break;
        case 909:
            rank_kernel<9, 9><<<blocks, threads>>>(begin, end, slices, ranks);
            break;
        case 1616:
            rank_kernel<16, 16><<<blocks, threads>>>(begin, end, slices, ranks);
            break;
        default:
            throw std::runtime_error("no kernel compiled for this shape");
    }
    GPU_LEAF_CHECK(cudaGetLastError());
}

}  // namespace

bool span_ranks_handle(std::size_t rows, std::size_t columns) {
    if (rows != columns) return false;
    return rows == 4 || rows == 5 || rows == 9 || rows == 16;
}

GpuRanks rank_span_on_gpu(const SpanQuestion& question, std::uint64_t begin, std::uint64_t end) {
    constexpr unsigned int kThreadsPerBlock = 256;
    constexpr std::uint64_t kElementsPerLaunch = 1ull << 24;
    if (question.slices > kSliceCeiling) {
        throw std::runtime_error("span of more slices than the kernel holds");
    }
    if (question.words > kWordCeiling) throw std::runtime_error("map wider than the kernel holds");
    if (end < begin) throw std::runtime_error("an empty range read backwards");

    GpuRanks result;
    result.ranks.resize(static_cast<std::size_t>(end - begin));
    if (end == begin) return result;

    DeviceBuffer ranks(result.ranks.size() * sizeof(unsigned char));
    Event opened, closed;

    const auto started = std::chrono::steady_clock::now();
    GPU_LEAF_CHECK(cudaMemcpyToSymbol(c_slice_rows, question.slice_rows.data(),
                                      question.slice_rows.size() * sizeof(std::uint64_t)));

    for (std::uint64_t first = begin; first < end; first += kElementsPerLaunch) {
        const std::uint64_t last = std::min<std::uint64_t>(first + kElementsPerLaunch, end);
        const unsigned int blocks =
            static_cast<unsigned int>((last - first + kThreadsPerBlock - 1) / kThreadsPerBlock);
        GPU_LEAF_CHECK(cudaEventRecord(opened.handle));
        // The output pointer is offset so a thread writes `index - begin`, which
        // is what the kernel computes: the launches tile the range and every
        // element lands in exactly one slot.
        launch(question, blocks, kThreadsPerBlock, first, last,
               static_cast<unsigned char*>(ranks.pointer) + (first - begin));
        GPU_LEAF_CHECK(cudaEventRecord(closed.handle));
        GPU_LEAF_CHECK(cudaEventSynchronize(closed.handle));
        result.kernel_seconds += seconds_between(opened, closed);
    }

    GPU_LEAF_CHECK(cudaMemcpy(result.ranks.data(), ranks.pointer,
                              result.ranks.size() * sizeof(unsigned char),
                              cudaMemcpyDeviceToHost));
    result.wall_seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    return result;
}

}  // namespace gpu_leaf
