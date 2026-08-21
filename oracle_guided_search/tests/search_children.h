#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"
#include "canonical_parent.h"
#include "pool_cosets.h"
#include "pool_set_canon.h"
#include "span_basis.h"
#include "subspace_canon.h"

/// The children the canonical enumeration puts through its parent test, handed to
/// a visitor.
///
/// This is [`canonical_augmentation.cpp`](../canonical_augmentation.cpp)'s
/// `descend` with the leaf work removed and a callback added: the same
/// orbit-pruned candidates and the same acceptance rule, so a check driven from
/// here sees the sets the search itself sees rather than sets that merely resemble
/// them. It is a header because two tests want it, and a second copy of a walk is
/// a copy free to drift from the first.
namespace search_children {

/// `visit(child)` for every child of `current`, then a descent into the ones the
/// parent test accepts. `base` is the subspace the walk starts from, so
/// `dimension` counts from its span and `target` stops the descent.
template <class Visit>
void below(const bilinear_rank::Field& field, const std::vector<bilinear_rank::Matrix>& base,
           const std::vector<bilinear_rank::Matrix>& pool,
           const bilinear_rank::PoolSetCanon& canon,
           const std::vector<bilinear_rank::Matrix>& current, std::size_t dimension,
           std::size_t target, const Visit& visit) {
    if (dimension >= target) return;

    const bilinear_rank::SubspaceCode current_code = bilinear_rank::subspace_code(field, current);

    // One `PoolCosets` a node, as the search itself builds: it answers what is
    // inside this subspace, what is outside it, and what is inside each child.
    const bilinear_rank::PoolCosets cosets(field, pool, current);
    // One candidate per orbit of the current subspace's stabiliser, which is what
    // makes this the search's tree and not a wider one.
    const std::vector<std::vector<std::uint32_t>> action =
        canon.stabiliser_generators(cosets.inside());

    for (const std::uint32_t index :
         bilinear_rank::orbit_representatives(action, cosets.outside())) {
        std::vector<bilinear_rank::Matrix> child = current;
        child.push_back(pool[index]);
        visit(child);
        const bilinear_rank::ParentTest test = bilinear_rank::is_canonical_augmentation(
            field, base, child, current_code, index, cosets.extended_by(index), pool, canon);
        if (!test.accepted) continue;
        below(field, base, pool, canon, child, dimension + 1, target, visit);
    }
}

}  // namespace search_children
