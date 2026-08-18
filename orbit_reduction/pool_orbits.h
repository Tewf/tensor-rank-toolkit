#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// The orbits of the rank-one pool, found on the vectors rather than on their
/// products.
///
/// The group is `[covanov2019, Def. 7]`'s action and the orbits are lossless to
/// quotient by because it preserves rank, `[covanov2019, Prop. 9]`; keys are
/// [`../references.md`](../references.md).
///
/// The pool is every outer product `u·vᵀ` of a normalised left vector with a
/// normalised right vector, so it is a grid: `(p^rows − 1)/(p−1)` by
/// `(p^columns − 1)/(p−1)`. And the action never mixes the two sides,
///
///     σ · (u vᵀ) = μᵀ (u vᵀ) ν = (μᵀu)(νᵀv)ᵀ
///
/// so an automorphism is a pair of permutations, one per side, not one
/// permutation of the grid. That is the whole idea here, and it is worth a great
/// deal: at `⟨3,3,3⟩` the grid is 261 121 matrices of 81 entries, 184 MB, and
/// the two sides are 511 vectors each.
///
/// Orbits of pairs then come from orbit-stabiliser rather than from walking the
/// grid: take one left vector per orbit, and orbit the right vectors under the
/// subgroup fixing it. The grid is never enumerated, which is what makes the
/// question askable at shapes where the grid does not fit in memory at all.
namespace bilinear_rank {

/// An automorphism as it acts on the pool: one permutation of the left vectors
/// and one of the right.
struct FactoredAction {
    std::vector<std::vector<std::uint32_t>> left;
    std::vector<std::vector<std::uint32_t>> right;
};

/// How each element of `group` permutes the two vector lists.
FactoredAction factored_action(const Field& field, const std::vector<Automorphism>& group,
                               std::size_t rows, std::size_t columns);

/// One pool index per orbit, as `left_index * right_count + right_index`, which
/// is the order `all_rank_one_maps` builds the pool in.
std::vector<std::uint32_t> pool_orbit_representatives(const Field& field,
                                                      const std::vector<Automorphism>& group,
                                                      const FactoredAction& action,
                                                      std::size_t rows, std::size_t columns,
                                                      std::size_t left_count,
                                                      std::size_t right_count);

/// The representatives themselves, and only those: the pool is never built.
std::vector<Matrix> rank_one_orbit_representatives(const Field& field,
                                                   const std::vector<Automorphism>& group,
                                                   std::size_t rows, std::size_t columns);

/// The same orbits for a matrix multiplication tensor, written down instead of
/// computed.
///
/// **This closed form is this repository's own, and the citation it used to
/// carry was wrong.** `[covanov2019, Cor. 18]` says that elements *of the
/// target subspace* `T` of a given rank lie in one orbit, and the pool is not
/// inside `T`: a slice of `⟨n,m,k⟩` has rank `m`, so for `m > 1` nothing in `T`
/// is rank one at all. What is taken from the paper is the group and nothing
/// else, `[covanov2019, Thm. 17]`, the stabiliser as the pairs
/// `(P ⊗ Rᵀ, Q ⊗ R⁻¹)`.
///
/// The rest is derived here and asserted by the tests rather than cited. A
/// rank-one map of `⟨n,m,k⟩` is a pair `(U, V)`, `U` an `n×m` matrix and `V` an
/// `m×k` one, and that group acts by change of basis on each side sharing the
/// middle: `U ↦ P U R` and `V ↦ R⁻¹ V Q`. So `rank U`, `rank V` and `rank UV`
/// are all invariant, the middle basis cancelling in the product. That those
/// three are a *complete* invariant, with `rank UV` confined to
/// `max(0, rU+rV−m) ≤ t ≤ min(rU, rV)`, is the claim the closed form rests on
/// and it is stated nowhere but here.
///
/// Counting those triples gives 5 for `⟨2,2,2⟩` and 13 for `⟨3,3,3⟩`, which is
/// what the general computation returns. No group is built, no vector list is
/// walked, and nothing is enumerated: the answer is a triple loop.
std::vector<Matrix> matrix_multiplication_orbit_representatives(const Field& field, std::size_t rows,
                                                      std::size_t inner, std::size_t columns);

/// The same representatives as the two vectors whose outer product they are.
///
/// A rank-one map is `u vᵀ`, and a consumer that wants to constrain the two
/// operands separately, such as a formula with one variable per coordinate,
/// needs the vectors rather than the product. Over GF(2) the pair is unique, so
/// nothing is lost either way.
std::vector<std::pair<std::vector<Element>, std::vector<Element>>>
matrix_multiplication_orbit_vectors(const Field& field, std::size_t rows, std::size_t inner,
                                    std::size_t columns);

}  // namespace bilinear_rank
