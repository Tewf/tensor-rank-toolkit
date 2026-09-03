#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

/// The group of a grid `L × R`, presented on `L ⊔ R` rather than on the grid.
///
/// [`pool_set_canon.h`](pool_set_canon.h) asks the group two things about a set of
/// grid cells: its lexicographically least image, and the subgroup fixing it
/// setwise. `[permlib]` answers both, but only about subsets of the domain the
/// group is presented on, and presenting on the grid costs a permutation of
/// `left_count * right_count` points per generator: 900 bytes at `⟨2,2,2⟩`, a
/// megabyte at `⟨3,3,3⟩`, **17 GB** at `⟨4,4,4⟩`.
///
/// The action never mixes the two sides (`[covanov2019, Thm. 17]`), and
/// [`pool_orbits.h`](../orbit_reduction/pool_orbits.h) sets the algebra out, so a
/// generator is a pair of permutations, one of each vector list, and `G` embeds in
/// `Sym(L) × Sym(R)`. Presenting it on the **disjoint union** `L ⊔ R` costs
/// `left_count + right_count` points instead: 131 070 at `⟨4,4,4⟩`, half a megabyte
/// a generator. That is the whole content of this file; the rest is the reduction
/// of the two questions from the product action to that presentation.
///
/// **The structural fact the reduction rests on.** In the product action the
/// stabiliser of one cell is the intersection of the two coordinate stabilisers,
///
///     Stab_G((l, r)) = Stab_G(l) ∩ Stab_G(r),
///
/// because `g = (λ, ρ)` fixes `(l, r)` exactly when `λ(l) = l` and `ρ(r) = r`. So a
/// stabiliser chain of `G` on `L ⊔ R` whose base is extended by a row point and a
/// column point at a time induces, level by level, the chain of cell stabilisers a
/// search on the product action asks for. One BSGS of degree
/// `left_count + right_count` is enough and the grid is never a domain.
///
/// **Why `[permlib]`'s own search is not called.** `OrbitLexMinSearch` is written
/// against subsets of the BSGS's own domain: `dset`s of size `bsgs.n`, a
/// `DSetAction`, per-level caches sized `n`. Nothing in it is parameterised over
/// the acted-on set, so the product action cannot be handed to it, and
/// `vendor/` is not patched. `[linton2004]`'s algorithm is therefore written out
/// again in [`factored_lex_min.cpp`](factored_lex_min.cpp) against **cells**,
/// reusing `[permlib]`'s `BSGS`, `ConjugatingBaseChange`, `RandomBaseTranspose`,
/// `PointwiseStabilizerPredicate`, `SchreierTreeTransversal` and classic backtrack
/// unchanged. It is the same algorithm and it returns the same answer: the flat
/// index `left * right_count + right` numbers the cells exactly as the grid
/// presentation numbered its points, so the least image is the same set of
/// indices and not merely an equally good choice of one.
/// [`tests/test_factored_canonisation.cpp`](tests/test_factored_canonisation.cpp)
/// holds the two against each other on every shape where the grid presentation
/// still fits in memory.
///
/// **One file for two questions, deliberately.** They share the presentation, and
/// splitting them would either build the BSGS twice or put a `[permlib]` type in a
/// header, which is the thing the pimpl below exists to prevent.
namespace bilinear_rank {

/// An element of the group as it acts on the grid: one permutation of the left
/// vector list and one of the right.
///
/// The same shape as one column of
/// [`pool_orbits.h`](../orbit_reduction/pool_orbits.h)'s `FactoredAction`, which is
/// where the generators arrive from, and deliberately not that type: a stabiliser
/// found here is handed back in the form it was given, so both ends of the module
/// speak one language.
struct FactoredGenerator {
    std::vector<std::uint32_t> left;
    std::vector<std::uint32_t> right;
};

/// What presenting the group on an axis point costs, near enough.
///
/// Carried over unchanged from the grid presentation, where `⟨3,3,3⟩`'s 261 121
/// points measured at about 370 bytes each. The construction is the same
/// Schreier-Sims with the same Schreier tree transversals on a smaller domain, so
/// the *rate* is the one thing that does not change when the degree does, but no
/// new measurement stands behind the number, which is why that is said here rather
/// than left implied.
///
/// It is priced through `require_room` like every other bulk allocation here, so a
/// shape too large refuses with a sentence and `--max-memory` moves it. At
/// `⟨4,4,4⟩`'s 131 070 axis points that is about 48 MB and no refusal, where the
/// grid's 4 294 836 225 points priced at 1.6 TB and always refused.
inline constexpr std::size_t kBytesPerAxisPoint = 370;

class FactoredGrid {
   public:
    /// `generators` permute the two lists; an empty list is the trivial group and
    /// nothing is presented at all.
    FactoredGrid(std::size_t left_count, std::size_t right_count,
                 const std::vector<FactoredGenerator>& generators);
    ~FactoredGrid();
    FactoredGrid(FactoredGrid&&) noexcept;
    FactoredGrid& operator=(FactoredGrid&&) noexcept;

    std::size_t left_count() const;
    std::size_t right_count() const;
    /// `left_count() * right_count()`. A count and never a domain: nothing here
    /// allocates one of those.
    std::size_t cell_count() const;

    /// The lexicographically least image of `cells` under the group, as sorted flat
    /// indices `left * right_count() + right`.
    ///
    /// Least is by the sorted index sequence compared entry by entry, which for
    /// sets of one size is the order `[permlib]`'s `smallestSetImage` puts on
    /// subsets. Equal answers mean one orbit; unequal answers mean two.
    ///
    /// `const`, and it copies the BSGS it changes the base of, so the enumeration's
    /// worker threads may all be inside it at once.
    std::vector<std::size_t> least_image(const std::vector<std::size_t>& cells) const;

    /// Generators of the subgroup fixing `cells` **setwise**, factored the way they
    /// were given.
    ///
    /// The cheap answer here is the wrong one and is not taken. The subgroup fixing
    /// the set of touched rows and the set of touched columns is one call to
    /// `setStabilizer` on this presentation, and it is in general **larger** than
    /// `Stab_G(S)`, because it may carry cells out of the set and others in:
    /// measured larger on 13.6% of 20 000 random cell sets of one to nine cells at
    /// `⟨2,2,2⟩`, and 12 elements against 1 at the worst of them. Too
    /// large is the unsound direction: it merges augmentations that are not
    /// equivalent, and a refutation built on it is a wrong lower bound that nothing
    /// downstream can catch. So that group is used only to **prune** the backtrack,
    /// where being too permissive costs time and nothing else, and membership is
    /// decided on the cells themselves.
    std::vector<FactoredGenerator> setwise_stabiliser(const std::vector<std::size_t>& cells) const;

   private:
    /// PermLib types stay out of this header: every includer would otherwise
    /// inherit a vendored library and Boost.
    struct Presentation;
    std::unique_ptr<Presentation> presentation_;
};

}  // namespace bilinear_rank
