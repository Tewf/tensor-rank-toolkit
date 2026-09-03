#pragma once

#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "candidate_pool.h"
#include "gf2_bits.h"
#include "gf2_span_basis.h"
#include "host_reference.h"
#include "leaf_question.h"

/// Whether the card and the shipped leaf answered the same question the same
/// way, element for element.
///
/// Two things are compared and neither implies the other. The **survivor set**
/// is every candidate that passed the test, which is what a kernel produces and
/// what [`host_reference.h`](host_reference.h) produces; equal sizes would not
/// be evidence, so the indices are compared one by one. The **rank-one basis**
/// is what the leaf hands its caller, the greedy having run over the survivors,
/// and it is what a search would act on.
///
/// The greedy is the reason this file can exist at all. `contains` is a pure
/// function of the candidate and the span, and the span does not change while a
/// scan runs, so which candidates survive does not depend on the order they are
/// tested in. Only the greedy depends on order, and running it on the host over
/// survivors sorted by index visits exactly the indices the sequential loop
/// would have visited, in the same order, so it keeps the same maps.
namespace gpu_leaf {

inline bool same_maps(const std::vector<bilinear_rank::Matrix>& left,
                      const std::vector<bilinear_rank::Matrix>& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index].rows() != right[index].rows()) return false;
        if (left[index].columns() != right[index].columns()) return false;
        for (std::size_t entry = 0; entry < left[index].entry_count(); ++entry) {
            if (left[index].data()[entry] != right[index].data()[entry]) return false;
        }
    }
    return true;
}

/// The maps a pool scan would have kept, from the survivor indices alone.
///
/// The `found.size() + (pool_.size() - index) < needed` break of
/// [`Gf2Leaf::by_scanning_the_pool`](../../methods/bilinear_rank/exhaustive/gf2_leaf.h) is
/// reproduced rather than ignored: it can only fire once what is left of the
/// pool is shorter than what is still needed, so testing it at each survivor
/// stops on the same survivor the sequential loop stops before.
inline std::vector<bilinear_rank::Matrix> maps_kept_from_scan(
    const bilinear_rank::RankOnePool& pool, const LeafQuestion& question,
    const std::vector<std::uint64_t>& survivors, std::size_t needed) {
    linear_algebra::Gf2SpanBasis independent(question.width);
    std::vector<std::uint64_t> candidate(question.words), scratch(question.words);
    std::vector<bilinear_rank::Matrix> kept;
    for (const std::uint64_t index : survivors) {
        if (kept.size() + (question.pool_size() - index) < needed) break;
        packed_outer_product(question, question.left_masks[index / question.right_count],
                             question.right_masks[index % question.right_count], candidate.data());
        if (!independent.try_add(candidate.data(), scratch)) continue;
        kept.push_back(pool.at(static_cast<std::size_t>(index)));
        if (kept.size() == needed) break;
    }
    return kept;
}

/// The maps a subspace walk would have kept, from the survivor indices alone.
inline std::vector<bilinear_rank::Matrix> maps_kept_from_walk(
    const LeafQuestion& question, const std::vector<std::uint64_t>& survivors,
    std::size_t needed) {
    linear_algebra::Gf2SpanBasis independent(question.width);
    std::vector<std::uint64_t> element(question.words), scratch(question.words);
    std::vector<bilinear_rank::Matrix> kept;
    for (const std::uint64_t index : survivors) {
        subspace_element(question, index, element.data());
        if (!independent.try_add(element.data(), scratch)) continue;
        bilinear_rank::Matrix map(question.rows, question.columns);
        linear_algebra::gf2_unpack(element.data(), question.width, map.data());
        kept.push_back(std::move(map));
        if (kept.size() == needed) break;
    }
    return kept;
}

}  // namespace gpu_leaf
