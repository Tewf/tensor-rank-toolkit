#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "subspace_canon.h"

/// Deciding, from a subspace alone, which of its parents it should have been reached
/// from.
///
/// This is the half of McKay canonical augmentation (`[mckay1998]`) that has to be
/// **group-invariant or worthless**: if the choice of parent depended on how the
/// object was built, two members of one orbit could choose differently and both
/// survive. So nothing here looks at the walk's history. Keys are
/// [references.md](../references.md).
namespace bilinear_rank {

/// Whether an augmentation is the canonical one, and what deciding it cost.
///
/// The cost is returned rather than hidden because it is the whole open question:
/// the test walks the group once per candidate parent, and at a handful of
/// representatives per level that may be dearer than the duplicates it removes.
struct ParentTest {
    bool accepted = false;
    std::size_t group_visits = 0;
};

/// Whether `child`, which is `parent + added`, should be accepted as reached from
/// that parent.
///
/// Two conditions and both are needed.
///
/// 1. `parent` is in the class of the **canonical parent**, the candidate parent of
///    least canonical code. Candidate parents are the hyperplanes of
///    `span(child) / span(base)` that are themselves reachable, that is spanned by
///    `base` plus pool elements of the child.
/// 2. `added` is the **distinguished** pool element. Its key is the least code of its
///    image under a group element carrying `child` to canonical form, minimised over
///    all such elements so the key cannot depend on which one is picked. Two elements
///    with equal keys are carried onto each other by an automorphism of `child`, so
///    the least key is attained on exactly one orbit.
///
/// Without the second condition, two orbits of `added` under the parent's stabiliser
/// can build the same child class and both would pass. With it, exactly one
/// augmentation survives per class, which is what makes the enumeration isomorph-free
/// rather than merely thinner.
ParentTest is_canonical_augmentation(const Field& field, const std::vector<Matrix>& base,
                                    const std::vector<Matrix>& child,
                                    const SubspaceCode& parent_code, const Matrix& added,
                                    const std::vector<Matrix>& pool,
                                    const std::vector<Automorphism>& group);

}  // namespace bilinear_rank
