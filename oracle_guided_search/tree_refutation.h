#pragma once

#include <cstddef>
#include <vector>

#include "automorphism.h"
#include "strict_deflation.h"

/// Refuting a pinned rank question with the quotiented tree instead of a solver.
///
/// `[covanov2019]` Algorithm 3, as `expand_subspace_up_to_symmetry`, asked about the map's
/// span **enlarged by the candidate**. It decides the same thing the cube does and
/// prices it very differently: unpinned at `⟨2,2,2⟩` and `k = 6` the tree and kissat
/// are comparable, 0.41 s against 0.31 s, but **pinned the tree takes 0.0085 to
/// 0.0099 s against the solver's 0.29 to 0.35 s**.
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
CandidateVerdict tree_verdict(const Field& field, const linear_algebra::Tensor& tensor,
                              std::size_t products, const Matrix& candidate,
                              const std::vector<Matrix>& pool,
                              const std::vector<Automorphism>& ambient, std::size_t node_limit,
                              std::vector<Matrix>& decomposition);

}  // namespace bilinear_rank
