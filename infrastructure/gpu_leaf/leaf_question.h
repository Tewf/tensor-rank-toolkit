#pragma once

#include <cstdint>
#include <vector>

/// One leaf of the exact search, in the representation a kernel reads.
///
/// The leaf test asks the same thing of every candidate, and the only things it
/// needs to know are the shape, the span it is testing membership in, and the
/// two vector lists the pool is the outer-product grid of. All of that is under
/// 300 KB at the widest shape this repository prices, which is why a kernel over
/// it needs no transfer per element: see [`README.md`](README.md).
///
/// **Plain data, and no Givaro.** This header is included by the `.cu` files,
/// which nvcc compiles with its own host front end; Givaro's headers do not
/// survive that, and a kernel has no use for exact arithmetic anyway.
/// [`question_packing.h`](question_packing.h) is where this is built from the
/// repository's own types, and only C++ translation units include that.
namespace gpu_leaf {

/// The shape, the span, and the two vector lists, packed one bit per entry.
///
/// `left_masks[i]` is the `rows`-bit pattern of the `i`th normalised left
/// vector and `right_masks[j]` the `columns`-bit pattern of the `j`th right
/// one, so pool element `i * right_count + j` is their outer product and is
/// derived rather than stored. That is the same index
/// [`RankOnePool::at`](../../methods/bilinear_rank/candidate_pool.h) uses.
///
/// `span_rows` is `dimension` rows of `words` words, in the order
/// `SpanBasis::rows()` hands them over, which is the order the subspace walk
/// reads its digits in. `pivots[j]` is the leading set bit of row `j`, which is
/// the pivot [`Gf2SpanBasis`](../../core/linear_algebra/gf2_span_basis.h) would find,
/// because the rows arrive already in reduced row echelon form.
struct LeafQuestion {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::size_t width = 0;
    std::size_t words = 0;
    std::size_t left_count = 0;
    std::size_t right_count = 0;
    std::vector<std::uint32_t> left_masks;
    std::vector<std::uint32_t> right_masks;
    std::vector<std::uint64_t> span_rows;
    std::vector<std::uint32_t> pivots;

    std::size_t dimension() const { return pivots.size(); }
    std::size_t pool_size() const { return left_count * right_count; }
};

}  // namespace gpu_leaf
