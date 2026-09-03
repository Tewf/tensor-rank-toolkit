#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"

/// The pool content of a subspace **and of every subspace one pool element above
/// it**, from one scan instead of one per child.
///
/// **This is the cost the canonical route was bound by, and it was not the group.**
/// Measured at five shapes
/// ([`when-canonical-pays/where-the-time-goes.md`](when-canonical-pays/where-the-time-goes.md)),
/// a canonical node spent 7 to 14 whole pool scans: 98% of its time at `<2,3,3>`,
/// against 1.0% in the setwise stabiliser and 1.1% in the canonical image. Most of
/// those scans were one per **candidate child**, because
/// `is_canonical_augmentation` opened with `pool_inside(field, pool, child)` and
/// every child of a node differs from it in one pool element.
///
/// **The structure that removes them.** `span(child) = span(current) + <added>`, so
/// a pool element lies in the child exactly when it lies in the current subspace or
/// its residue modulo that subspace spans the same line as `added`'s. Reduce every
/// pool element once, group the nonzero residues by their normalised value, and the
/// content of a child is the current content merged with one of those groups: a
/// lookup, whatever the number of children. One scan a node replaces `c + 2` of
/// them.
///
/// **It is the same list, not an equally good one.** `inside()` is
/// `pool_inside(field, pool, generators)` and `extended_by(i)` is
/// `pool_inside(field, pool, generators + pool[i])`, both increasing, both exact;
/// [`tests/test_pool_cosets.cpp`](tests/test_pool_cosets.cpp) holds them against
/// the scan on every subspace the search visits rather than on a sample.
namespace bilinear_rank {

class PoolCosets {
   public:
    /// One reduction per pool element against `span(generators)`.
    PoolCosets(const Field& field, const std::vector<Matrix>& pool,
               const std::vector<Matrix>& generators);

    /// Pool indices inside `span(generators)`, increasing.
    const std::vector<std::size_t>& inside() const { return inside_; }

    /// Pool indices outside it, increasing. What `augmentations` scanned for
    /// separately, and the same scan produces both.
    const std::vector<std::uint32_t>& outside() const { return outside_; }

    /// The pool content of `span(generators) + pool[added]`, increasing.
    ///
    /// `added` must be outside the subspace, which is what the caller has just
    /// chosen it to be: a child that adds nothing is not a child.
    std::vector<std::size_t> extended_by(std::size_t added) const;

   private:
    std::vector<std::size_t> inside_;
    std::vector<std::uint32_t> outside_;
    /// Which coset each pool element's residue falls in, or `no_coset` for the ones
    /// inside the subspace.
    std::vector<std::size_t> coset_of_;
    /// The pool indices of each coset, increasing within a coset.
    std::vector<std::vector<std::size_t>> cosets_;
};

}  // namespace bilinear_rank
