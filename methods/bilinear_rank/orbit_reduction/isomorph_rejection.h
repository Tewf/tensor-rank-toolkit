#pragma once

#include <cstdint>
#include <vector>

#include "pool_orbits.h"

/// Which of a node's candidates open a branch, and how hard that is tested.
///
/// [`orbit_search.h`](orbit_search.h) walks a tree whose every node picks a pool
/// element out of the live suffix `[from, |P|)`. Two elements in one orbit of the
/// group still standing there open two branches that answer the same question,
/// so a rejection rule lets only some of them through. There are two rules here,
/// a search reads whichever the process was told to, and what separates them is
/// exactness against price.
///
/// **`Full` is exact rejection**, `[covanov2019, Alg. 3]` line 6, and it is the
/// rule every published number in this repository was taken with. It walks the
/// orbit of the candidate under the surviving group elements.
///
/// **`Generators` is partial rejection**: a candidate is struck out only when one
/// surviving element, applied **once**, lands it earlier in the live suffix.
/// `|residual|` lookups against an orbit search, and it lets through candidates
/// `Full` strikes out, which is duplicated work rather than a wrong answer.
///
/// That trade is the field's settled position rather than an invention here.
/// Complete rejection is barrier-hard: `[anders2024, Thm. 1.1]` puts graph
/// isomorphism in co-NP if a polynomial-time complete symmetry-breaking predicate
/// for row-column symmetries exists, and the barrier survives changing the order,
/// giving the predicate as a circuit, and adding variables. Partial rejection is
/// what practical systems use, and `[katsirelos2010]` is the paper that insists
/// the residual duplication be **counted** rather than assumed small, together
/// with the finding that a stronger break is not thereby a faster one. Keys are
/// [`../references.md`](../../../references.md).
///
/// **Counted here, and it does not pay**: 5.10x and 17.96x the nodes on the two
/// refutations measured, against a per-node surcharge for exactness that was
/// already only about 1.1x to 1.3x. The table, and why the satisfiable questions look
/// deceptively cheap, is in
/// [`what-partial-rejection-leaves.md`](what-partial-rejection-leaves.md). So
/// `Full` stays the default, and this switch exists to make that a measurement
/// rather than a remark.
///
/// ## Why the cheap rule cannot lose a solution
///
/// Not "some representative of every orbit survives", which is true and is weaker
/// than what is needed: the search narrows `residual` and advances `from` as it
/// descends, so a claim about one node in isolation says nothing about the tree
/// under it. What holds is a containment, and it is stronger.
///
/// Write `R` for `residual`, `O(i)` for the orbit of `i` under the group `R`
/// generates, and `L = [from, |P|)` for the live suffix. Note `i ∈ L` always,
/// since the loop starts at `from`.
///
/// 1. **`Full` opens `i` exactly when `i = min(O(i) ∩ L)`.** `least_in_orbit`
///    reaches all of `O(i)`: it composes words in `R` forwards only, but every
///    element of a finite group has finite order, so `g⁻¹ = g^(ord(g)−1)` is
///    already among those words and the submonoid it walks is the subgroup. It
///    then refuses `i` when a member of `O(i)` lies in `[from, i)`, and "no
///    member of `O(i) ∩ L` below `i`" is minimality.
///
/// 2. **Everything `Full` opens, `Generators` opens.** Let `i = min(O(i) ∩ L)`
///    and take any `g ∈ R`. Then `g·i ∈ O(i)`, so were `g·i` in `[from, i)` it
///    would be a member of `O(i) ∩ L` strictly below the minimum. It is not, so
///    no single element of `R` strikes `i` out, and `Generators` opens it.
///
/// 3. **The two trees nest, node for node.** A node is `(span, from, residual)`,
///    and a child's triple is computed from its parent's and from the candidate
///    alone: `from' = chosen`, and `residual'` keeps the elements of `residual`
///    whose image of `chosen` landed inside the enlarged span. Neither reads the
///    rejection rule, and nor does the containment test that skips a candidate
///    already spanned. So by induction on depth, every node the `Full` walk
///    visits the `Generators` walk visits too, carrying the identical triple, and
///    the `Generators` walk visits some others besides.
///
/// A walk over a superset of a sound walk's tree cannot miss what the sound one
/// finds, and a `FOUND` is checked against the map it must compute. So the two
/// verdicts agree. The extra nodes can cost only the budget: `Generators` can
/// reach a `--node-limit` where `Full` did not, which is an undecided rather than
/// a wrong answer.
///
/// **Step 2 is where a hole would be, so it is worth saying what would open
/// one.** Nothing there asks `R` to be a generating set of anything, to be closed
/// under composition, or to contain inverses; `g·i ∈ O(i)` is the whole of it,
/// and that holds for any `g ∈ R` by the definition of `O`. What it does need is
/// that both rules read the **same** `R` and the **same** `from`, which they do,
/// because the narrowing above is rule-blind. A rule that struck out the minimum
/// would break exactly this step, and
/// [`tests/test_generator_rejection.cpp`](tests/test_generator_rejection.cpp)
/// sabotages it that way to show the coverage check can see it happen.
namespace bilinear_rank {

/// Which rejection rule a quotiented search applies.
///
/// Process-wide, like `set_leaf_route` and `set_worker_count`, read once per
/// candidate and written by nothing but a command line. `Full` is the default and
/// must stay it: it is the rule behind every node count recorded here.
enum class OrbitTest { Full, Generators };
void set_orbit_test(OrbitTest test);
OrbitTest orbit_test();

/// Exact: is `point` the least member of its orbit inside `[from, |pool|)`?
///
/// **Breadth first, because `residual` may be a generating set.** Applying each
/// element once reaches part of the orbit and leaves the rest, which is sound and
/// wastes exactly what the quotient is for, and it is what `Generators` below
/// deliberately does instead.
///
/// **The `>= from` guard is the whole of the restriction.** The question is
/// least-in-orbit *among the live candidates* rather than least outright. Without
/// the guard a branch whose smaller twin was consumed by an ancestor is skipped,
/// and the solutions under it go with it.
bool least_in_orbit(const PoolAction& action, const std::vector<std::uint32_t>& residual,
                    std::uint32_t point, std::uint32_t from);

/// Partial: does no single element of `residual` send `point` earlier in
/// `[from, |pool|)`?
///
/// `|residual|` lookups and no orbit. True wherever `least_in_orbit` is true, by
/// step 2 above, and true in places besides.
bool least_under_generators(const PoolAction& action, const std::vector<std::uint32_t>& residual,
                            std::uint32_t point, std::uint32_t from);

/// Whichever of the two the process was told to use.
bool opens_a_branch(const PoolAction& action, const std::vector<std::uint32_t>& residual,
                    std::uint32_t point, std::uint32_t from);

}  // namespace bilinear_rank
