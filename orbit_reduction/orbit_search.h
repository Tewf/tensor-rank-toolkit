#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "exhaustive_search.h"
#include "bilinear_rank_aliases.h"

/// The exact search with its tree quotiented by a group of automorphisms.
///
/// `[covanov2019, Alg. 3]`, `BDEZStab`, which recurses on a target subspace,
/// its stabiliser and a target rank, and returns one representative per
/// equivalence class. The plain search
/// ([`exhaustive_search.h`](../exhaustive_search/exhaustive_search.h)) is
/// `[bdez2012, Alg. 1]`; this is the same decision, visiting one branch per
/// orbit instead of one per candidate. Keys are
/// [`../references.md`](../references.md).
///
/// What makes the quotient lossless is `[covanov2019, Prop. 14]`: if a rank-`r`
/// decomposition of `T` meets the orbit `φ ∘ Stab(T)` at all, then some
/// decomposition equivalent to it contains `φ` itself. So a branch per orbit
/// misses no decomposition, only repetitions of one.
///
/// The saving is the size of the group, and the group is large exactly where
/// the questions are open: 216 elements for `⟨2,2,2⟩` over GF(2), about four for
/// polynomial multiplication.
namespace bilinear_rank {

/// Is there an algorithm with exactly `target` products, searched up to `group`?
///
/// Same answer as `expand_subspace`, fewer nodes. Two preconditions, and both
/// are checked rather than assumed:
///
/// - **`group` must stabilise `span(subspace)`.** Pass what `stabiliser_of`
///   returned. A group that does not is the one way this can report a `NO` that
///   is false, and a false `NO` is the only claim here that nothing downstream
///   can catch: a `FOUND` is checked against the map it must compute.
/// - **`pool` must be closed under `group`**, which `all_rank_one_maps` is.
///   `permutation_action_on` throws if an image leaves it.
bool expand_subspace_up_to_symmetry(const Field& field, const std::vector<Matrix>& subspace,
                           const std::vector<Matrix>& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products);

}  // namespace bilinear_rank
