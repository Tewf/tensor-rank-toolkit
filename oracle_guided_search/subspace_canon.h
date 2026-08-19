#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// Naming a subspace so that two of them can be compared, and so that a whole
/// orbit of them has one name.
///
/// An enumerator that wants to visit one subspace per orbit needs to recognise the
/// same subspace twice, and a subspace has no canonical basis until one is chosen.
/// `SpanBasis` already keeps its rows in reduced row echelon form, so the only
/// thing missing is a fixed row order; sorting by pivot column supplies it, and the
/// result is unique to the subspace rather than to the order its generators arrived
/// in.
///
/// Naming an **orbit** rather than a subspace is a different question and lives
/// in [`pool_set_canon.h`](pool_set_canon.h), which answers it from generators.
namespace bilinear_rank {

/// A subspace's name. `Element` is `int64_t`, so codes compare lexicographically
/// with no conversion.
using SubspaceCode = std::vector<Element>;

/// The reduced basis of `span(generators)`, rows ordered by pivot column, laid end
/// to end. Equal codes mean equal subspaces.
SubspaceCode subspace_code(const Field& field, const std::vector<Matrix>& generators);

/// **The orbit walk that used to live here is gone.** `canonical_subspace` named
/// an orbit by taking the least code over every element of the group, and
/// `image_code` served the distinguished-element test that walked the attaining
/// coset. [`pool_set_canon.h`](pool_set_canon.h) answers both questions by
/// canonical image under a prescribed permutation group and dominates them on the
/// same field: measured on `<2,2,2>` at target 7, 954 nodes and 21.9 s became 83
/// nodes and 3.04 s, and no group element is walked at all.
///
/// Dominated code goes to the `rejected-experiments` branch rather than staying
/// here, which is what happened to `find-at-rank`. The evidence stays:
/// [`deduplication-cost.md`](deduplication-cost.md) carries the before and after,
/// and this file's history carries the code.

}  // namespace bilinear_rank
