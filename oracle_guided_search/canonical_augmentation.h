#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "subspace_canon.h"
#include "tensor_file.h"

/// Generating each solution subspace once per orbit instead of once per orbit
/// element.
///
/// `expand_subspace`'s `from` index deduplicates **orderings** only: a set of pool
/// elements is reached as `a < b < c` rather than `3!` times. It does nothing about
/// two genuinely different subspaces that are the same under the group. Both get
/// built and both get searched, so the work below them is repeated once per orbit
/// element. **Nothing in this repository deduplicated up to the group**, and
/// `[bdez2012]` §4.5 and `[covanov2019]` Algorithm 7 both say duplicates have to
/// go, neither implementing it. Keys are [references.md](../references.md).
///
/// The standard fix is **McKay canonical augmentation** (`[mckay1998]`, the `nauty`
/// lineage): give every object a **canonical parent**, computed from the object
/// alone by a group-invariant rule, and accept an augmentation `Y -> X` only when
/// `parent(X)` is the class of `Y`. Every class is then reached from exactly one
/// place.
///
/// **The reason it is this and not a set of everything seen** is memory. A seen-set
/// costs one canonical form per object generated, which at these counts is hopeless.
/// The parent test is local to a node, so storage is constant per node and nothing
/// accumulates.
///
/// **The validation target is known, and it is not an opinion.** `⟨2,2,2⟩` at 7 has
/// exactly **36** solution subspaces in exactly **1** orbit under the 216-element
/// group, with the rank-one elements they use being 90 of 225, computed twice and
/// agreeing. So the plain enumerator must return 36 and this one must return 1.
/// Anything else is a bug, and `tests/test_canonical_augmentation.cpp` asserts both.
///
/// **What it costs is a separate question from whether it works**, and it is asked
/// rather than assumed: the parent test walks the group once per node, so at five to
/// twenty-six representatives per level the saving may not cover it. Both node
/// counts and both wall clocks are reported so the crossover can be found.
namespace bilinear_rank {

/// What an enumeration visited and found.
struct EnumerationReport {
    /// Solution subspaces emitted, counting each path that reached one.
    ///
    /// Larger than `distinct` when `canonical` is off, because the ordering
    /// constraint stops a *sequence* of pool elements repeating and not a *set* of
    /// them: a subspace whose seven contained rank-one maps span a three-dimensional
    /// quotient is reached once per basis of that quotient. At `⟨2,2,2⟩` that is 720
    /// paths to 36 subspaces.
    std::size_t emitted = 0;
    /// Distinct subspaces among those emitted, found by keeping every code seen.
    ///
    /// **This is the number the canonical route exists to avoid paying for.** Getting
    /// 36 out of the plain enumerator needs one canonical code stored per solution
    /// found, which is the O(objects) memory McKay's local test replaces with nothing.
    std::size_t distinct = 0;
    /// Subspaces built and tested, the measure that does not depend on the machine.
    std::size_t nodes = 0;
    /// Group elements looked at by the parent test. **Zero since the parent test
    /// stopped walking the group**, which is what it measured; it is kept rather
    /// than removed because `deduplication-cost.md` publishes it, and a column
    /// going to zero says what happened where a renamed one would not.
    std::size_t group_visits = 0;
    /// Canonical images asked for, which is what replaced the walk. One per
    /// candidate parent and one per pool element of the child.
    std::size_t canonisations = 0;
    double seconds = 0;
    /// One rank-one basis per emitted subspace, each already checked against the map
    /// by `recovers_map`.
    std::vector<std::vector<Matrix>> decompositions;
};

/// Enumerate the `target`-dimensional subspaces containing `span(tensor)` that have
/// a rank-one basis drawn from `pool`.
///
/// `canonical` off is the repository's existing behaviour, orderings deduplicated
/// and nothing else. On, it is McKay canonical augmentation against `group`, which
/// must contain the identity's effect on the pool, that is `pool` must be closed
/// under it; `all_rank_one_maps` is and `rank_one_candidates` is not.
///
/// An empty `group` makes the canonical route degenerate to the plain one, since
/// every object is then its own orbit. Reported rather than refused, because that is
/// exactly what happens at shapes whose group will not fit in a list.
///
/// `stop_at_first` returns as soon as one solution is in hand, for a caller that is
/// **deciding** rather than counting. Off by default, and it has to be: `emitted`,
/// `distinct` and `nodes` are all counts of a completed walk, and a walk that
/// stopped early reports numbers that mean nothing. It exists because a rank search
/// asks only whether the level is empty, and finishing a level it has already
/// answered is work spent on a question nobody asked.
///
/// **`run_limits::worker_count()` above one spreads the walk's subtrees over cores, and changes
/// no count.** This is the one search here where that is a fact rather
/// than a hope: it counts instead of stopping, so there is no witness to race to
/// and no shared budget to spend against, and every subtree is visited whatever
/// the thread count. `emitted`, `distinct`, `nodes` and `canonisations` are
/// therefore identical at one thread and at twelve, asserted in
/// `tests/test_canonical_augmentation.cpp`; contrast
/// [`what-threads-change.md`](../exhaustive_search/what-threads-change.md), where
/// the shared budget makes a thread count visible in a verdict. `stop_at_first`
/// is excluded and stays sequential, because a walk that stops early is exactly
/// the case the invariance argument does not cover.
///
/// Which decomposition stands for a subspace, and the order `decompositions`
/// arrives in, do depend on the split: a subspace reached down two branches is
/// kept once, from the lower-numbered branch. The count does not. What the split
/// is worth in wall clock, and where it stops paying:
/// [`enumerating-on-every-core.md`](enumerating-on-every-core.md).
EnumerationReport enumerate_solution_subspaces(const Field& field,
                                               const formats::Tensor& tensor,
                                               const std::vector<Matrix>& pool,
                                               const std::vector<Automorphism>& group,
                                               std::size_t target, bool canonical,
                                               bool stop_at_first = false);

}  // namespace bilinear_rank
