#pragma once

#include <cstddef>
#include <vector>

#include "candidate_pool.h"
#include "automorphism.h"
#include "exhaustive_search.h"
#include "bilinear_rank_aliases.h"

/// The exact search with its tree quotiented by a group of automorphisms.
///
/// `[covanov2019, Alg. 3]`, `BDEZStab`, which recurses on a target subspace,
/// its stabiliser and a target rank, and returns one representative per
/// equivalence class. The plain search
/// ([`exhaustive_search.h`](../exhaustive_search/exhaustive_search.h)) is
/// `[bdez2012, Alg. 1]`; this is the same decision, visiting one branch per
/// orbit instead of one per candidate. Keys are
/// [`../references.md`](../references.md).
///
/// What makes the quotient lossless is `[covanov2019, Prop. 14]`: if a rank-`r`
/// decomposition of `T` meets the orbit `φ ∘ Stab(T)` at all, then some
/// decomposition equivalent to it contains `φ` itself. So a branch per orbit
/// misses no decomposition, only repetitions of one.
///
/// The saving is the size of the group, and the group is large exactly where
/// the questions are open: 216 elements for `⟨2,2,2⟩` over GF(2), about four for
/// polynomial multiplication.
///
/// **Which of a node's candidates open a branch is not decided here.** That is
/// [`isomorph_rejection.h`](isomorph_rejection.h), which carries both the exact
/// rule this search has always used and the cheap partial one `--orbit-test
/// generators` selects, and the argument that swapping them changes node counts
/// and never a verdict.
namespace bilinear_rank {

/// Is there an algorithm with exactly `target` products, searched up to `group`?
///
/// Same answer as `expand_subspace`, fewer nodes. Two preconditions, and both
/// are checked rather than assumed:
///
/// - **`group` must stabilise `span(subspace)`.** Pass what `stabiliser_of`
///   returned. A group that does not is the one way this can report a `NO` that
///   is false, and a false `NO` is the only claim here that nothing downstream
///   can catch: a `FOUND` is checked against the map it must compute.
/// - **`pool` must be closed under `group`**, which `all_rank_one_maps` is.
///   `permutation_action_on` throws if an image leaves it.
///
/// **`worker_count()` above one spreads the subtrees over cores, and the split
/// leaves both preconditions alone.** The group is filtered by `stabiliser_of`
/// once, before any worker exists, and the permutation action is read-only from
/// then on, so the one way this search can report a false `NO` is not something a
/// worker can reach. What the workers do share is the budget, and that is
/// visible: a refutation walks the same tree whoever walks it, so its node total
/// is exact at any thread count, while a witness stops the search early and the
/// subtrees already in flight spend against the same counter, so that total is an
/// upper bound and a tight `--node-limit` can turn a proof into an undecided.
/// Measured, with the mitigation this search now carries too:
/// [`what-threads-change.md`](../exhaustive_search/what-threads-change.md).
///
/// The split cannot be at the root the way the plain search's is. The plain
/// search has one first choice per pool element, 225 at `⟨2,2,2⟩`; the quotient
/// leaves one per orbit, which is 5, and collapsing them is the entire point of
/// being here. So the frontier is widened a node at a time until it is at least as
/// wide as the worker count.
///
/// `spread_over_cores` off keeps this on one core whatever `worker_count()` says,
/// for a caller that is **already** spreading work over them. `deflate-strictly
/// --parallel` asks every candidate at once and each candidate calls this, so
/// without the switch twelve workers would each start eleven more. The outer level
/// is the one to keep: it has one branch per candidate and no shared state, where
/// this one shares a budget and races to a witness.
bool expand_subspace_up_to_symmetry(const Field& field, const std::vector<Matrix>& subspace,
                           const std::vector<Matrix>& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products,
                           bool spread_over_cores = true, SearchTrace* trace = nullptr);

/// The same quotiented walk over an **addressed** pool, which is never held.
///
/// The precondition is met for the same reason it is met by `all_rank_one_maps`:
/// this is the same grid of outer products, indexed rather than stored, and
/// `PoolAction` reads the group's action off the two vector lists it is built
/// from. So a shape whose pool is 8.2 TiB can be quotiented, where before the
/// two were exclusive and `--symmetry` was refused there outright.
bool expand_subspace_up_to_symmetry(const Field& field, const std::vector<Matrix>& subspace,
                           const RankOnePool& pool,
                           const std::vector<Automorphism>& group, std::size_t target,
                           SearchBudget& budget, std::vector<Matrix>& products,
                           bool spread_over_cores = true, SearchTrace* trace = nullptr);

}  // namespace bilinear_rank
