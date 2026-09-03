#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "strict_deflation.h"

/// Refuting a pinned rank question with the quotiented tree instead of a solver.
///
/// `[covanov2019]` Algorithm 3, as `expand_subspace_up_to_symmetry`, asked about the map's
/// span **enlarged by the candidate**. It decides the same thing the cube does and
/// prices it very differently. Measured together at `⟨2,2,2⟩` and `k = 6`, before
/// 2026-08-20: unpinned the tree took **0.41 s** against kissat's 0.31 s, and
/// pinned it took **0.0085 to 0.0099 s** against the solver's 0.29 to 0.35 s. So
/// **pinning is worth about forty-five times to the tree and nothing at all to the
/// solver**, and that is the comparison this file argues from, one session against
/// itself.
///
/// **What can be refreshed has been, and what cannot is said rather than left to
/// look current.** The unpinned pair is retaken and both its ends are published:
/// the tree is **0.0291 s** against kissat's **0.339 s**, both read out of the one
/// row of `satisfiability/results.json` that carries the pair, so the two are not
/// *comparable*, which this comment used to say, and it is the tree that is ahead
/// by **11.7x**. Both ends come from that row rather than one from each file
/// because the same question is timed in two of them, and a ratio assembled from
/// two runs is not a ratio: this comment quoted the tree out of
/// `methods/bilinear_rank/greedy_heuristic/results.json` and kissat out of the file above, and read 11.6x
/// where the pair reads 11.7x. It
/// moved because the GF(2) bit-packed leaf and the reflected Gray walk landed on
/// 2026-08-20 and the solver's second is another program's. **The pinned pair
/// nothing re-measures**: no results file carries a pinned question, so both of
/// its ends are quoted as taken and the tree's end of it is an upper bound under
/// the leaf that ships. Forty-five times is therefore the floor of what pinning
/// is worth here, not a figure that has aged into being wrong.
///
/// The reason is that enlarging the span removes a level of the tree. `span(T) + t`
/// has dimension 5 at `⟨2,2,2⟩` and the target is 6, so one level is left and 45
/// orbits are the whole search. Pinning does nothing comparable to a CNF instance.
/// The advantage therefore scales with `k - dim(span(T) + t)` and disappears when
/// that is large.
///
/// **It refutes something slightly stronger than the cube.** A cube pins the
/// candidate as a *term*; this only requires it to lie in the *span*. Refuting the
/// weaker statement refutes the stronger, so a rejection here is sound. That is the
/// safe direction of the two and worth naming, because the other direction would
/// produce a false lower bound.
namespace bilinear_rank {

/// The ambient group as a list, or empty when it will not fit in one.
///
/// `matrix_multiplication_symmetries` refuses when the list will not fit the
/// **memory budget**, which is a documented refusal and not an error: `⟨2,2,2⟩`
/// over GF(2) is 216 elements and `⟨3,3,3⟩` is 4 741 632, about 6.2 GiB against a
/// 2 GiB default. Without a group the tree decides the same question and visits
/// one branch per candidate rather than one per orbit, so falling back costs
/// pruning and never correctness.
///
/// **It is a budget and not a ceiling, so `--max-memory` defeats it**, and at
/// `⟨3,3,3⟩` that is the worse outcome rather than the better one: the list is
/// then built, and `canonical_subspace` walks all 4.7 million of it once per
/// candidate parent. Raising the budget here buys a run that does not end.
std::vector<Automorphism> ambient_or_empty(const Field& field,
                                           const std::vector<std::size_t>& shape);

/// Whether any `products`-term decomposition of `tensor` has `candidate` in its
/// span, decided by the tree.
///
/// `Yes` fills `decomposition`. `No` is a refutation. `Unknown` means the node limit
/// was hit, which proves nothing in either direction.
///
/// `spread_over_cores` is passed straight to the tree and is off when the caller is
/// already asking every candidate at once, so that the two levels do not both fan
/// out; [`../orbit_reduction/orbit_search.h`](../orbit_reduction/orbit_search.h)
/// says why the outer one wins.
CandidateVerdict tree_verdict(const Field& field, const formats::Tensor& tensor,
                              std::size_t products, const Matrix& candidate,
                              const std::vector<Matrix>& pool,
                              const std::vector<Automorphism>& ambient, std::size_t node_limit,
                              std::vector<Matrix>& decomposition, bool spread_over_cores = true);

}  // namespace bilinear_rank
