#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// One move per orbit, so a node opens one branch where it opened several that
/// answer the same question.
///
/// **Why a node's moves may be quotiented at all.** If `σ` maps `span(V)` to
/// itself then `σ(V + <g>) = V + <σg>`, and `σ` preserves the rank of every form
/// and of every subspace (`[covanov2019, Prop. 9]`; keys are
/// [`../references.md`](../references.md)), so the two children cost the same and
/// `σ` carries the whole subtree above one onto the whole subtree above the
/// other. Whatever cost is reachable through `g` is reachable through `σg`, so
/// entering one of each orbit loses nothing the search was looking for. That is
/// the argument [`../orbit_reduction/orbit_heuristic.h`](../orbit_reduction/orbit_heuristic.h)
/// makes for a pool at a fixed map, carried up the tree the way
/// `[covanov2019, Alg. 3]` carries it down its recursion.
///
/// **The group has to be the node's own, and that is the line that keeps this
/// honest.** `Stab(span(T))` is what makes two moves interchangeable at the
/// root; one adjunction later the span is larger and its stabiliser is a
/// different, usually smaller, group. A quotient taken once and reused would
/// throw away the move that would have paid, so the stabiliser is recomputed
/// from the node's own basis at every node, which is also why this is priced
/// per node rather than once per run.
///
/// **The move set is closed under that group**, which is what
/// [`permutation_action_on`](../orbit_reduction/automorphism.h) requires and
/// refuses to proceed without. For the generated moves:
/// [`level_lowering_moves`](level_lowering_moves.h) offers every rank-one `g`
/// with `rank(v - g) = rank(v) - 1` for `v` in the span of rank between two and
/// the cutoff, and nothing else. `σ` sends such a `g` to a rank-one `σg` with
/// `rank(σv - σg) = rank(σv) - 1`, `σv` is back in the span with the same rank,
/// and `σg` is outside the span exactly when `g` was. So the set maps onto
/// itself. For `--whole-pool` it is `all_rank_one_maps`, which the same header
/// records as closed.
namespace bilinear_rank {

/// The moves that open a branch: one per orbit of the subgroup of `ambient`
/// stabilising `span(slices)`, in the order they were offered.
///
/// `ambient` empty returns `moves` untouched, which is what every caller that
/// was never given a group reads as "try every move". `stabiliser_size`, when
/// not null, receives how large that subgroup turned out to be, worth knowing
/// because a stabiliser of one quotients nothing and the run should be able to
/// say so rather than leave a reader to infer it from an unchanged count.
///
/// **Lowest index per orbit**, which is `orbit_representatives`' rule and
/// matters here: the children are sorted cheapest first with ties in generation
/// order, so keeping the earliest member of each orbit keeps the move the
/// unquotiented search would have met first.
std::vector<Matrix> moves_up_to_symmetry(const Field& field, const std::vector<Matrix>& slices,
                                         const std::vector<Automorphism>& ambient,
                                         const std::vector<Matrix>& moves,
                                         std::size_t* stabiliser_size);

}  // namespace bilinear_rank
