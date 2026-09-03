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
/// [its orbits](orbit_reduction/pool_orbits.h) are computed on the lists rather than on the
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
/// [`pool_orbits.h`](orbit_reduction/pool_orbits.h) keys its orbit tables by,
/// so the two are interchangeable elementwise and
/// [`greedy_heuristic/tests/test_candidate_pool.cpp`](greedy_heuristic/tests/test_candidate_pool.cpp) asserts it.
///
/// Step 3 walks this rather than the grid on its unquotiented path, and so does
/// `walk-scheme --from`, which is what [`improving_candidates`](greedy_heuristic/minimise_rank.h)
/// has a second overload for: on the 9x9 slices of `⟨3,3,3⟩` that is 16.7 MB of
/// resident memory against 189.4 MB, measured both ways at the same cut-off.
///
/// **The exact search now walks it too**, and the objection that kept it from doing
/// so turned out to be worth about a fifth. It carries a pool index down its
/// recursion and resumes from it, so an addressed pool rebuilds a map once per
/// node that reaches an index rather than once per run, and that was predicted
/// here to "trade the memory for the search". Measured on `⟨3,3,3⟩` at target 23
/// with the same 300-node cut-off, one core:
///
/// | pool | time | peak resident |
/// |---|---|---|
/// | materialised | **93.6 s** | 181 880 kB |
/// | addressed | 112.9 s | **4 776 kB** |
///
/// **38x less memory for 1.21x the time.** So `decide-rank` keeps the
/// materialised pool where it fits, which reproduces every published timing
/// exactly, and addresses it where the materialised one would be refused
/// outright: at `⟨4,4,4⟩` that is the difference between a search and a refusal
/// in milliseconds. This is `[yang2025]`'s odometer, whose whole difference from
/// `[bdez2012]` Algorithm 1 is that it never holds the pool.
///
/// Two callers still materialise unconditionally and are not converted: canonical
/// augmentation reads an index again on the way back up for its parent test, and
/// the quotiented step 3 keys its orbit tables by pool position. Both would need
/// the index to mean the same thing after a rebuild, which it does, but both
/// revisit far more often than the plain recursion does.
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

/// Where a search's candidates come from, so a walk is written once and works
/// against either pool.
///
/// A materialised pool hands back its own matrix and an addressed one builds the
/// matrix on the spot, and `const Matrix& map = candidates[index]` is correct for
/// both: the first aliases, the second extends the temporary's lifetime to the end
/// of the enclosing scope. Nothing in a walk can tell them apart, which is the
/// point of the two structs being this small.
///
/// They live here rather than in either search because both use them, and the
/// alternative was one copy each.
struct Materialised {
    const std::vector<Matrix>& maps;

    std::size_t size() const { return maps.size(); }
    const Matrix& operator[](std::size_t index) const { return maps[index]; }
};

struct Addressed {
    const RankOnePool& pool;

    std::size_t size() const { return pool.size(); }
    Matrix operator[](std::size_t index) const { return pool.at(index); }
};

/// Whether the row space of `inner` sits inside that of `outer`.
///
/// Asks whether some `z` has `z · x = y` by comparing `rank(x)` with
/// `rank([xᵀ | yᵀ])`. Left multiplication cannot leave the row space, so that
/// comparison is exactly this containment.
bool row_space_contains(const Field& field, const Matrix& outer, const Matrix& inner);

/// Whether any of `known` has a row space containing `candidate`'s.
bool has_same_row_space(const Field& field, const std::vector<Matrix>& known, const Matrix& candidate);

/// One representative per row-space class of a pool of rank-one maps.
///
/// The write-up's conclusion names shrinking `G` by automorphisms as the way
/// forward. **It is not wired into either search**, and should not be until the
/// rest of the argument exists: replacing a candidate by a representative of
/// its class only preserves the answer if the same automorphism is applied to
/// the map being decomposed. Measuring the reduction is useful on its own, and
/// [`greedy_heuristic/tests/test_candidate_pool.cpp`](greedy_heuristic/tests/test_candidate_pool.cpp) reports it.
std::vector<Matrix> row_space_representatives(const Field& field, const std::vector<Matrix>& pool);

}  // namespace bilinear_rank
