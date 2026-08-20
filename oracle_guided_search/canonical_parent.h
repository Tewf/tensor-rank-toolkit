#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "pool_set_canon.h"
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
    /// Group elements walked. **Zero on the canonical-image route**, which walks
    /// none: it asks `[linton2004]` for the least image instead, and the count
    /// that replaces it is `canonisations`. Kept rather than renamed because
    /// `deduplication-cost.md` publishes it, and a column going to zero says what
    /// happened where a column quietly meaning something else would not.
    std::size_t group_visits = 0;
    /// Canonical images asked for. One per candidate parent, plus one for the
    /// distinguished-element test, which asks about the added element alone
    /// rather than about every pool element of the child: `is_distinguished_cell`
    /// says why the other cells' answers were already known.
    std::size_t canonisations = 0;
};

/// A candidate parent, and the pool elements lying inside it by index.
///
/// The two travel together because they are found together: deciding whether a
/// hyperplane is reachable at all means listing the pool elements it contains,
/// and that same list is what names its orbit. Splitting them left the caller
/// scanning the whole pool for a list it had just been handed.
struct CandidateParent {
    std::vector<Matrix> generators;
    /// Exactly `pool_inside(field, pool, generators)`, without the pool scan.
    /// `span(parent) ⊆ span(child)`, since every generator is a member of `base`
    /// or a combination of the added ones, so a pool element inside the parent is
    /// already inside the child and the child's content is the only place worth
    /// looking: one membership test per pool element of the child, where the scan
    /// is one per rank-one map of the shape and there are 261 121 at `⟨3,3,3⟩`.
    std::vector<std::size_t> content;
};

/// Every reachable subspace one dimension below `span(base) + span(added)` and
/// above `span(base)`, each with its pool content.
///
/// These are the hyperplanes of the quotient, one per nonzero linear functional up
/// to scalar, which `normalised_vectors` enumerates exactly. `added` is a basis of
/// that quotient; which basis does not matter, because the set of hyperplanes does
/// not depend on it, and that independence is what makes the canonical parent
/// invariant.
///
/// Reachable means spanned by `base` plus pool elements, since the walk can add
/// nothing else. A hyperplane that is not is no parent of anything. `inside` is
/// the child's pool content, which is where the parents' comes from.
///
/// Exposed rather than private so that
/// [`tests/`](tests/test_parent_pool_content.cpp) can hold `content` against the
/// scan it replaces on every parent the search visits.
std::vector<CandidateParent> candidate_parents(const Field& field,
                                               const std::vector<Matrix>& base,
                                               const std::vector<Matrix>& added,
                                               const std::vector<Matrix>& pool,
                                               const std::vector<std::size_t>& inside);

/// Whether `marked` is a distinguished cell of `indices`: one whose marked-pair
/// key is least over the cells of the set.
///
/// **Asked of one cell rather than of all of them.** The doubled ground set puts a
/// set's points below the mark's and `[permlib]` orders images by their sorted
/// points, so a key reads as `canon(indices)` followed by the mark's image, and
/// the first part is the same whichever cell is marked. Only the mark entry is
/// left to compare, and its least value over the cells is forced: the mark's image
/// lies in `canon(indices)`, and every point of `canon(indices)` is the image of
/// some cell, so the least attainable mark entry is that set's own least point —
/// which is the key's own first entry. A cell is therefore least exactly when its
/// own key says so, and the loop over the other cells was asking a question whose
/// answer it already held.
bool is_distinguished_cell(const PoolSetCanon& canon, const std::vector<std::size_t>& indices,
                           std::size_t marked);

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
/// `canon` replaces the walk. Built once for the whole enumeration, because its
/// presentation of the group costs more than one test does and every test wants
/// the same one.
ParentTest is_canonical_augmentation(const Field& field, const std::vector<Matrix>& base,
                                    const std::vector<Matrix>& child,
                                    const SubspaceCode& parent_code, const Matrix& added,
                                    const std::vector<Matrix>& pool,
                                    const PoolSetCanon& canon);

}  // namespace bilinear_rank
