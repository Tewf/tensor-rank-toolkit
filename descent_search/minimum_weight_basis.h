#pragma once

#include <vector>

#include "bilinear_rank_aliases.h"

/// Step 1, and the only step of the heuristic that guarantees anything.
///
/// Choosing a basis of `span(T)` whose ranks sum to the least is a **matroid**
/// problem, and none of that is new here. Keys are
/// [`../references.md`](../references.md).
///
/// In general: linear independence of vectors over a field is a matroid,
/// `[oxley, Prop. 1.1.1]`, and the greedy returns a maximum-weight basis of any
/// matroid under any weight function, `[oxley, Lem. 1.8.3]`; 1.8.2's remark
/// that solving for `-w` gives a minimum-weight one is what makes ascending
/// order the right direction here.
///
/// For this exact problem: `[nakatsukasa2017, Thm. 2.1]` states the same
/// exactness of the same greedy for the low-rank basis problem itself, and its
/// `Cor. 1` is stronger than the matroid statement, every basis of lowest rank
/// taking the same ranks up to permutation. Their *algorithm* is over the reals
/// and does not port; the theorem is the matroid argument and does.
///
/// So this does not approximate, and its answer does not depend on how ties are
/// broken. What is heuristic is the constraint that the answer be a basis of
/// `span(T)` at all, which is exactly what [steps 2 and 3](minimise_rank.h)
/// relax.
namespace bilinear_rank {

/// Walk the span of the map from lowest rank upwards, greedily keeping anything
/// not already spanned, until a full basis is assembled.
///
/// The result spans exactly what it was given, so it computes the same bilinear
/// map, and its ranks sum to no more than the original's.
///
/// `ranks_without_last` is an optimisation, and optional: the ranks of the span
/// of every slice but the last, in enumeration order. Steps 2 and 3 call this
/// once per candidate with the map fixed and the candidate appended, so the
/// `p^(k)` elements whose coefficient on the candidate is zero are the same
/// elements every single time. Handing them over skips both the combination and
/// the rank for that half of the enumeration.
///
/// **Most of the other half is skipped too, and no answer moves.** The dearest
/// element the greedy can take is bounded by the dearest of the slices it was
/// handed, so every element above that ceiling is dropped before it is ranked —
/// on the strength of `|rank(v) - rank(g)|`, which needs no elimination. The
/// argument and its two lines are in
/// [`minimum_weight_basis.cpp`](minimum_weight_basis.cpp); what it buys is 2 043
/// ranks a call down to 187 on `cyclic_f2_7`, and it is a claim about the answer
/// rather than about the search, so a basis that came up short under it throws
/// instead of being returned.
///
/// `cost` takes the answer's cost, which is the sum of the ranks the greedy
/// picked and is therefore already known when the basis is handed back.
/// [`multiplication_count`](../linear_algebra/measures.h) recovers the same
/// number by ranking every basis element again, and stays the right call for
/// anyone holding only matrices; a caller of this function is not one of those.
std::vector<Matrix> minimum_weight_basis(const Field& field, const std::vector<Matrix>& slices,
                                   const std::vector<std::size_t>& ranks_without_last = {},
                                   std::size_t* cost = nullptr);

/// The basis of `slices` with one more map thrown in: the answer to "what would
/// adopting this candidate cost?".
///
/// Both walks over a candidate pool ask exactly this, so it lives here with the
/// step it calls rather than twice in the two callers. `cost` is the answer to
/// the question as asked — the count, not the basis — and every caller here
/// wants it, which is why it comes back rather than being asked for again.
std::vector<Matrix> minimum_weight_basis_with(const Field& field, const std::vector<Matrix>& slices,
                               const Matrix& candidate,
                               const std::vector<std::size_t>& known = {},
                               std::size_t* cost = nullptr);

/// The rank of every element of the span, indexed the way `coefficient_vector`
/// indexes it, for feeding back in above.
///
/// **Walked in reflected Gray order, and that is invisible to the answer.** The
/// order the `p^k` elements are visited in decides only how each one is formed:
/// under [`ReflectedGrayWalk`](reflected_gray_walk.h) consecutive elements differ
/// by one slice added or subtracted, so forming one costs `O(width)` field
/// additions and no multiplication, against the `O(k * width)` multiply-
/// accumulates a rebuild from `coefficient_vector(index)` costs. The index is
/// carried alongside, `± p^digit` a step.
///
/// **Nothing downstream can see the order**, which is why this one was safe to
/// change and others were not. The result is written to `ranks[index]`, an
/// index-addressed slot, so the vector that comes out is the vector that came
/// out before, entry for entry, whatever sequence filled it. Every caller reads
/// it by index too: [`minimum_weight_basis`](minimum_weight_basis.h) above as
/// `ranks_without_last`, `SortedSpan` as its filtration, and
/// [`level_lowering_moves`](../incumbent_search/level_lowering_moves.h) as a
/// filter on which elements are cheap enough to split. Where a routine instead
/// *consumes* elements in order — a greedy taking the first that pays — this
/// order is not available and the fix is to collect and sort back, as
/// `Gf2Leaf::by_carrying_a_residual` does.
std::vector<std::size_t> span_element_ranks(const Field& field,
                                            const std::vector<Matrix>& slices);

}  // namespace bilinear_rank
