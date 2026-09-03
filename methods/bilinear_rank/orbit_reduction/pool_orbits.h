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

/// Where an automorphism sends a pool element, answered by arithmetic rather
/// than by a table.
///
/// [`automorphism.h`](automorphism.h)'s `permutation_action_on` answers the same
/// question from a stored table with **one entry per (automorphism, pool
/// element)**. That is affordable while the pool is, and it is the reason the
/// quotiented search cannot run on a shape whose pool is only addressed: at
/// `⟨4,4,4⟩` the table is tens of gigabytes per automorphism, before any search
/// begins.
///
/// It never needed a table. `[covanov2019, Thm. 17]`'s stabiliser moves the two
/// factors of a rank-one map on their own sides, so an element's image is one
/// lookup in each vector list and a multiply. Two lists of 65 535 against a grid
/// of 4 294 836 225 is **32 768x less** at that shape, and the two agree on every
/// element: [`tests/test_orbit_cubes.cpp`](tests/test_orbit_cubes.cpp) asserts it
/// against the table on `⟨2,2,2⟩`, all 48 600 images.
///
/// **GPU note.** This is the one lookup an accelerated leaf would need per
/// candidate, and it is branch-free, allocation-free and reads two small tables
/// every thread can share. See `writeup/positioning/hardware-and-parallelism.md`.
class PoolAction {
   public:
    PoolAction(const Field& field, const std::vector<Automorphism>& group, std::size_t rows,
               std::size_t columns);

    std::size_t size() const { return factored_.left.size(); }

    /// The image of pool index `index` under automorphism `element`.
    std::uint32_t image(std::size_t element, std::uint32_t index) const {
        return static_cast<std::uint32_t>(factored_.left[element][index / right_count_]) *
                   static_cast<std::uint32_t>(right_count_) +
               factored_.right[element][index % right_count_];
    }

   private:
    FactoredAction factored_;
    std::size_t right_count_ = 0;
};

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
/// **The citation this used to carry was wrong, and so was the note saying
/// nothing published covered it.** `[covanov2019, Cor. 18]` says that elements
/// *of the target subspace* `T` of a given rank lie in one orbit, and the pool
/// is not inside `T`: a slice of `⟨n,m,k⟩` has rank `m`, so for `m > 1` nothing
/// in `T` is rank one at all. What is taken from the paper is the group and
/// nothing else, `[covanov2019, Thm. 17]`, the stabiliser as the pairs
/// `(P ⊗ Rᵀ, Q ⊗ R⁻¹)`.
///
/// A rank-one map of `⟨n,m,k⟩` is a pair `(U, V)`, `U` an `n×m` matrix and `V`
/// an `m×k` one, and that group acts by change of basis on each side sharing
/// the middle: `U ↦ P U R` and `V ↦ R⁻¹ V Q`. So `rank U`, `rank V` and
/// `rank UV` are all invariant, the middle basis cancelling in the product.
///
/// **That the three are a *complete* invariant was long marked unproven here.
/// It is not: this is the `A_3` quiver, and Gabriel's theorem owns it.** Read
/// `U` as a map `K^m → K^n` and `V` as `K^k → K^m` and the pair is a
/// representation of `• → • → •` with dimension vector `(k, m, n)`; the group
/// above is exactly its base-change group, `R` acting on `U` from the right and
/// on `V` from the left by `R⁻¹`, which is what one change of basis at a shared
/// vertex does. Orbits are therefore isomorphism classes of representations.
///
/// `A_3` is a simply-laced Dynkin diagram, so `[brion2008, Thm. 2.4.3]` gives
/// finite representation type, indecomposables in bijection with the positive
/// roots, and each one determined by its dimension vector; for type `A_r` those
/// roots are the intervals `Σ_{ℓ=i}^{j} ε_ℓ`, so `A_3` has exactly six, the
/// interval representations. Writing their multiplicities `a₁, a₂, a₃` for the
/// simples and `b, c, e` for the intervals `[1,2]`, `[2,3]`, `[1,3]`,
///
///     rank V = b + e,  rank U = c + e,  rank UV = e,
///     k = a₁ + b + e,  m = a₂ + b + c + e,  n = a₃ + c + e,
///
/// which inverts to `e = t`, `b = rV − t`, `c = rU − t`, `a₁ = k − rV`,
/// `a₃ = n − rU` and `a₂ = m − rU − rV + t`. Krull-Schmidt makes the six
/// multiplicities a complete invariant, and that substitution is a bijection
/// onto the triples, so the triple is one too. The bounds are the same
/// arithmetic: `b, c ≥ 0` is `t ≤ min(rU, rV)` and `a₂ ≥ 0` is
/// `t ≥ rU + rV − m`, exactly the range the loop runs over, and every
/// admissible triple is realised by the direct sum with those multiplicities.
///
/// `[buchfulton1999]`'s condition (1.2) on a rank array `(r_ij)` for the same
/// quiver is the same inequalities reached independently: for a chain of two
/// arrows it reads `t ≤ rU`, `t ≤ rV` and `m − rU − rV + t ≥ 0`, and they add
/// that rank arrays satisfying (1.2) *"are the only conditions that can
/// actually occur"*, which is the existence half.
///
/// **What is still owed.** `[brion2008]` proves this over an algebraically
/// closed field and says on its first page that Gabriel's theorem holds over an
/// arbitrary one, pointing at Benson, *Representations and Cohomology I*, §4.7.
/// That sentence is what the `GF(p)` case rests on and **Benson has not been
/// read here**; `[gabriel1972]` itself has not been read either, only
/// `[brion2008]`'s account of it. Nothing else in the derivation touches the
/// field: the six representations are defined over the prime field and
/// Krull-Schmidt holds anywhere.
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
