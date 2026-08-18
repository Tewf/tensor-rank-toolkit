#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"

/// The rank-one maps a search is allowed to recombine.
///
/// Both searches need this and neither owns it, which is why it is not in
/// either of their files.
namespace bilinear_rank {

/// The rank-one maps making up `slices`: one multiplication each, and exactly
/// as many as the slices cost.
std::vector<Matrix> rank_one_candidates(const Field& field, const std::vector<Matrix>& slices);

/// Nonzero vectors whose leading nonzero entry is 1: exactly one per scalar
/// class, `(p^length − 1)/(p−1)` of them.
///
/// The pool below is the grid of outer products of two of these lists, and
/// [its orbits](../orbit_reduction/pool_orbits.h) are computed on the lists rather than on the
/// grid, so they are worth having by name.
std::vector<std::vector<int64_t>> normalised_vectors(const Field& field, std::size_t length);

/// The same pool addressed by index, without building it.
///
/// The pool is the grid of outer products of two lists of normalised vectors, so
/// element `i` is `lefts[i / rights.size()] ⊗ rights[i % rights.size()]` and can
/// be formed on demand. Holding the two lists instead of the grid is the
/// difference between `(p^rows - 1)/(p-1) + (p^columns - 1)/(p-1)` vectors and
/// their product in matrices: for the 16×16 slices of `⟨4,4,4⟩`, about 16 MB
/// against the 8.2 TiB `all_rank_one_maps` prices and refuses.
///
/// **The index is the same index.** `left * right_count + right` is the order
/// `all_rank_one_maps` builds in and the order
/// [`pool_orbits.h`](../orbit_reduction/pool_orbits.h) keys its orbit tables by,
/// so the two are interchangeable elementwise and
/// [`tests/test_candidate_pool.cpp`](tests/test_candidate_pool.cpp) asserts it.
///
/// This does not make either search lazy: both index a materialised pool today,
/// and the exact search carries a pool index down its recursion. It is the piece
/// they would need first, and it is what lets a caller visit the pool in order
/// without asking for room it has not got.
class RankOnePool {
   public:
    RankOnePool(const Field& field, std::size_t rows, std::size_t columns);

    std::size_t size() const { return lefts_.size() * rights_.size(); }
    Matrix at(std::size_t index) const;

   private:
    const Field& field_;
    std::size_t rows_;
    std::size_t columns_;
    std::vector<std::vector<int64_t>> lefts_;
    std::vector<std::vector<int64_t>> rights_;
};

/// Every rank-one map of the given shape, one per scalar class.
///
/// There are `(p^rows - 1)(p^columns - 1) / (p-1)^2` of them: 961 for 5×5 over
/// GF(2), 4732 for 3×6 over GF(3). Built as outer products of vectors
/// normalised to leading entry 1.
///
/// Written as a walk over `RankOnePool` rather than a second copy of the outer
/// product, so the addressed and the materialised pool cannot drift apart.
std::vector<Matrix> all_rank_one_maps(const Field& field, std::size_t rows, std::size_t columns);

/// Whether the row space of `inner` sits inside that of `outer`.
///
/// Asks whether some `z` has `z · x = y` by comparing `rank(x)` with
/// `rank([xᵀ | yᵀ])`. Left multiplication cannot leave the row space, so that
/// comparison is exactly this containment.
bool row_space_contains(const Field& field, const Matrix& outer, const Matrix& inner);

/// Whether any of `known` has a row space containing `candidate`'s.
bool has_equivalent(const Field& field, const std::vector<Matrix>& known, const Matrix& candidate);

/// One representative per row-space class of a pool of rank-one maps.
///
/// The write-up's conclusion names shrinking `G` by automorphisms as the way
/// forward. **It is not wired into either search**, and should not be until the
/// rest of the argument exists: replacing a candidate by a representative of
/// its class only preserves the answer if the same automorphism is applied to
/// the map being decomposed. Measuring the reduction is useful on its own, and
/// [`tests/test_candidate_pool.cpp`](tests/test_candidate_pool.cpp) reports it.
std::vector<Matrix> one_per_row_space(const Field& field, const std::vector<Matrix>& pool);

}  // namespace bilinear_rank
