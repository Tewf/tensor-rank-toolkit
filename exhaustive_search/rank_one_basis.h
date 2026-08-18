#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "exhaustive_search.h"
#include "gf2_leaf.h"

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
///
/// Templated on where the candidates come from, so the same code serves a
/// materialised pool and an addressed one: see `Materialised` and `Addressed` in
/// [`candidate_pool.h`](../descent_search/candidate_pool.h). Instantiated for
/// exactly those two in the source, so the definition stays out of this header.
///
/// Note that the cheaper of the two routes above, walking the subspace, touches
/// the pool only for the shape and the size. So the leaf is already pool-free
/// wherever it is the route taken, and an addressed pool costs nothing there.
///
/// **`binary` is the GF(2) case of both routes**, in
/// [`gf2_leaf.h`](gf2_leaf.h), which is where the search spends its life over
/// the field most fixtures here are written over. Passing `nullptr` is the
/// general path, unchanged and answering for every other field; the choice
/// between the two routes is made here either way, so the two fields cannot
/// come to disagree about what a leaf is. It defaults to `nullptr` so that a
/// caller which has not built one, such as
/// [the quotiented search](../orbit_reduction/orbit_search.h), reads the same
/// as it did.
///
/// **`budget` is what stops a leaf**, and both routes are unbounded without it.
/// Whichever is taken here is the search's whole cost at a large shape, and the
/// node limit bounds neither: see `SearchBudget::leaf_element_limit` in
/// [`exhaustive_search.h`](exhaustive_search.h) for the measurement that says by
/// how much.
template <typename Candidates>
std::vector<Matrix> rank_one_basis_of(const Field& field, const ReducedBasis& span,
                                      const Candidates& pool, std::size_t needed,
                                      std::vector<Element>& scratch,
                                      SearchBudget* budget = nullptr,
                                      const Gf2Leaf<Candidates>* binary = nullptr);

}  // namespace bilinear_rank
