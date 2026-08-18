#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

/// Does this subspace have a basis made of rank-one maps?
///
/// BDEZ's `HasRankOneBasis`, and the question every leaf of both searches asks:
/// a `k`-dimensional space with a rank-one basis **is** a `k`-multiplication
/// algorithm.
///
/// There are two ways to ask it and they differ by orders of magnitude:
///
/// - **Scan the pool**, testing each rank-one map for membership. Costs
///   `|pool|` membership tests. This is what the paper writes, `H ← G ∩ V`.
/// - **Walk the subspace**, testing each of its `p^dim` elements for rank one.
///
/// For `⟨3,3,3⟩` at dimension 11 that is 2 048 rank computations against
/// 261 121 membership tests, and the leaf is where an exhaustive search spends
/// its life. For F2 5x5 it is the other way round: 961 pool elements against
/// 4 096 subspace elements. So the cheaper route is chosen per call rather than
/// picked once.
namespace bilinear_rank {

/// Up to `needed` independent rank-one maps inside `span`. Fewer than `needed`
/// means there is no such basis.
/// The shape comes from the pool, which is every rank-one map of it.
std::vector<Matrix> rank_one_basis_of(const Field& field, const ReducedBasis& span,
                                      const std::vector<Matrix>& pool, std::size_t needed,
                                      std::vector<Element>& scratch);

}  // namespace bilinear_rank
