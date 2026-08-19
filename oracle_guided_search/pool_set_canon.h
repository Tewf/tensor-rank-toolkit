#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "automorphism.h"
#include "bilinear_rank_aliases.h"

/// Naming the orbit of a set of pool indices, from generators, without walking
/// the group.
///
/// [`subspace_canon.h`](subspace_canon.h) names a subspace's orbit by taking the
/// least code over **every element** of the group. That is why it is a tool only
/// for shapes whose group fits in a list: at `⟨3,3,3⟩` the group is 4 741 632
/// elements and the walk happens once per candidate parent.
///
/// This names the same orbits from generators instead. The field's name for the
/// primitive is **canonical image of a set under a prescribed permutation
/// group**, it was solved by `[linton2004]`, and `[permlib]` implements it, so
/// nothing here implements a search: this file is the reduction and only that.
///
/// **Why a set of pool indices rather than the subspace.** The group acts on the
/// pool by permuting it, which `[covanov2019, Thm. 17]`'s factorisation makes
/// cheap: one permutation of the left vector list and one of the right, so
/// element `left * right_count + right` moves to
/// `left_image * right_count + right_image`. So `G` embeds in `Sym(pool)` and the
/// degree is the pool size, not the group order. That distinction is the whole
/// point: `f2_5x5`'s group is 9.99872e13 elements, which no walk can touch and
/// which `matrix_multiplication_symmetries` rightly refuses, and its permutation
/// degree is **961**.
///
/// **Why this asks the same question as the subspace code.** The enumerator
/// descends from `span(T)`, so every subspace it reaches is
/// `span(T) + span(chosen)` with `chosen` drawn from the pool, and therefore
/// `U = span(T) + span(pool ∩ U)`. That map from pool content to subspace is
/// injective on what the search visits, and it is `G`-equivariant because
/// `G = Stab(T)` fixes `span(T)`. Two subspaces are in one orbit exactly when
/// their pool contents are, so canonising the content canonises the subspace.
/// **This argument needs `G` to fix the base**, so it holds for
/// [`canonical_parent.cpp`](canonical_parent.cpp) and not for
/// [`tree_refutation.h`](tree_refutation.h), whose base the group does not fix.
///
/// **nauty is the wrong instrument, not a slower one.** It canonises under the
/// automorphism group it discovers for itself. Presented with this object that
/// group is `Sym(left) × Sym(right)`, which is larger than `G`, so it would merge
/// orbits `G` separates and discard subspaces that are genuinely inequivalent.
/// The result would be a lower bound with nothing downstream able to catch it.
/// `libnauty-dev` is installable here; that is not the reason it is unused.
namespace bilinear_rank {

/// The pool elements lying inside `span(generators)`, by index.
///
/// This is the object whose orbit `PoolSetCanon` names, so it lives beside it
/// rather than privately inside [`canonical_parent.cpp`](canonical_parent.cpp),
/// which is where it used to be and which left the two definitions of "the pool
/// content of a subspace" free to drift apart.
std::vector<std::size_t> pool_inside(const Field& field, const std::vector<Matrix>& pool,
                                     const std::vector<Matrix>& generators);

/// What presenting the group on a pool of `points` costs, near enough.
///
/// **Measured rather than derived**: `⟨3,3,3⟩` at 261 121 points with six
/// generators peaks at 97 MB and takes 29.8 s to present, which is about 370
/// bytes a point. A permutation itself is only four bytes a point, so the
/// transversals are almost all of it, and pricing on the permutations alone would
/// have been out by two orders of magnitude.
///
/// It is priced through `require_room` like every other bulk allocation here, so
/// a shape too large refuses with the same sentence and `--max-memory` moves it.
/// At `⟨4,4,4⟩`'s 4 294 836 225 points that is about 1.6 TB and a refusal.
inline constexpr std::size_t kBytesPerPoolPoint = 370;

class PoolSetCanon {
   public:
    /// Built from **generators**, which is the point. `rows` and `columns` are the
    /// slice shape, so the pool is the grid of outer products of normalised
    /// vectors and `size()` is its length.
    PoolSetCanon(const Field& field, const std::vector<Automorphism>& generators,
                 std::size_t rows, std::size_t columns);
    ~PoolSetCanon();
    PoolSetCanon(PoolSetCanon&&) noexcept;
    PoolSetCanon& operator=(PoolSetCanon&&) noexcept;

    /// The number of points the group is presented on: the pool size.
    std::size_t size() const;

    /// The lexicographically least image of `indices` under the group, as sorted
    /// pool indices. Equal answers mean one orbit.
    ///
    /// Least is by the bitset order `[permlib]` uses, which is a fixed total
    /// order on subsets of a fixed ground set; which order it is does not matter,
    /// only that it is the same one every time.
    std::vector<std::size_t> canonical(const std::vector<std::size_t>& indices) const;

    /// The canonical form of the **pair** (`indices`, `marked`), for choosing one
    /// pool element of a subspace in a way the group agrees with.
    ///
    /// The parent test needs a distinguished element of the child, and "least
    /// under the elements that carry the child to its canonical form" is how
    /// `canonical_parent.cpp` used to say it, which needs the attaining coset and
    /// therefore the whole group. Linton's algorithm hands back an image and not a
    /// canonising element, so that route is closed.
    ///
    /// The standard way round it is to canonise the pair instead of the set. A
    /// pair becomes a set on a **doubled ground set**: point `p` for membership
    /// and point `p + size()` to mark. The group acts the same way on both copies,
    /// so an orbit of pairs is an orbit of sets and the same primitive answers.
    /// Comparing these over the candidate elements picks the distinguished one,
    /// and it is the group's choice rather than an arbitrary basis's.
    std::vector<std::size_t> canonical_with_marked(const std::vector<std::size_t>& indices,
                                                   std::size_t marked) const;

    /// Generators of the subgroup fixing `indices` setwise, as permutations of
    /// the pool.
    ///
    /// **This is the last `|G|` dependency the canonical route had.**
    /// `stabiliser_of` filtered a group held as a list, so the canonical route
    /// could not run where the list could not be held: `⟨3,3,3⟩` is 4 741 632
    /// elements and 6.2 GiB, and it refused. Filtering *generators* instead is not
    /// an option and never was, because the generators that happen to fix a thing
    /// do not generate its stabiliser, which `pool_orbits.cpp` learned by getting
    /// 41 orbits where there are 13. A backtrack search does generate it, and
    /// `[permlib]`'s `setStabilizer` is one.
    ///
    /// **Why the setwise stabiliser of the pool content is the stabiliser of the
    /// subspace.** The enumerator descends from `span(T)`, so every subspace it
    /// reaches is `U = span(T) + span(pool ∩ U)`. If `g` fixes `U` it permutes the
    /// pool and fixes `U`, so it fixes `pool ∩ U`; and if `g` fixes `pool ∩ U` it
    /// fixes its span, and `g ∈ G = Stab(span T)` fixes `span(T)`, so it fixes the
    /// sum. The two groups are equal, and this asks for the one that is cheap.
    ///
    /// Each generator is returned as a full permutation of the pool, so this is
    /// `pool.size()` per generator: about a megabyte each at `⟨3,3,3⟩` and not
    /// affordable at `⟨4,4,4⟩`, where the caller would want the images computed
    /// rather than stored, as [`pool_orbits.h`](../orbit_reduction/pool_orbits.h)'s
    /// `PoolAction` does for the ambient group.
    std::vector<std::vector<std::uint32_t>> stabiliser_generators(
        const std::vector<std::size_t>& indices) const;

   private:
    /// PermLib types stay out of this header: every includer would otherwise
    /// inherit a vendored library and Boost.
    struct Presentation;
    std::unique_ptr<Presentation> presentation_;
};

}  // namespace bilinear_rank
