#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"

namespace pencil_rank {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

/// A lower bound by projecting the slice axis onto every plane.
///
/// This is `[yang2025thesis]`'s `rref` pruner at `k = 2`, read from the author's
/// notebook rather than the paper. The argument, in the form used here:
///
/// Let `T` have `n0` slices and suppose it has a rank-`R` decomposition whose
/// first-axis vectors span the whole of `GF(p)^n0`. Project that axis onto a
/// plane `P`, which is a `2 x n0` matrix `M` applied to the slices. The
/// projection kills `n0 - 2` independent directions, so a decomposition of
/// `M T` with at most `R - n0 + 2` terms exists **for enough planes that their
/// wedge products span the whole of the second exterior power**. Find that the
/// planes which could possibly qualify wedge to something smaller, and no
/// rank-`R` decomposition exists.
///
/// **What makes it affordable here is that `M T` is a pencil.** Yang's notebook
/// runs a CPD search per plane. Two slices is exactly the shape
/// [`kronecker_structure`](kronecker_structure.h) settles by exact linear
/// algebra in microseconds, so the inner question costs a canonical form rather
/// than a search. That is a saving available in this repository and not in the
/// source, and it is why `k = 2` is the case implemented.
///
/// **The bound is used in the safe direction.** `pencil_rank_of` returns a lower
/// bound on the pencil's rank, exact only when the pencil is diagonalisable over
/// GF(p). A plane is admitted whenever that bound fails to rule it out, so the
/// admitted set is a **superset** of the planes that truly qualify. A superset
/// spans at least as much, so a refutation here is sound; what it costs is
/// refutations missed, never a wrong one.
///
/// Returns true only when no decomposition with `products` terms exists. False
/// means undecided and never means one does.
bool projections_refute(const ModularField& field, const std::vector<ModularMatrix>& slices,
                        std::size_t products);

}  // namespace pencil_rank
