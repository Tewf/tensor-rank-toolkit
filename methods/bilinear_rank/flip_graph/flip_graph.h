#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "algorithm_recovery.h"
#include "bilinear_rank_aliases.h"

/// Walking a decomposition without changing what it computes.
///
/// `[kauers2023]`, in its 2022 form. Every other search here builds a
/// decomposition out of a pool of rank-one maps; this one starts from a
/// decomposition that already works and moves it.
///
/// The method has moved a long way since: symmetry `[moosbauer2025]`, orbits
/// `[ikenmeyer2025]`, adaptive and meta variants, and an open-source framework
/// `[perminov2026]` covering 680 formats. This implementation is behind all of
/// them and is not a contribution; what it is for, and the one question in this
/// area that is genuinely open here, is
/// [`writeup/positioning/`](../positioning/README.md).
///
/// Two moves, and the asymmetry between them is the whole method:
///
/// - a **flip** takes two terms sharing a factor and rewrites them, using
///   `A⊗B⊗Γ + A⊗B'⊗Γ' = A⊗(B+B')⊗Γ + A⊗B'⊗(Γ'−Γ)`. The sum is unchanged and so
///   is the rank, so a flip is free: it moves sideways for ever.
/// - a **reduction** fires when two terms come to share *two* factors. They
///   merge, and the rank drops by one.
///
/// Flips are common and reductions are rare, so the walk is: flip at random
/// until a reduction becomes available, take it, repeat. That is why this can
/// reach schemes a descending search cannot: the path between two good schemes
/// runs entirely through schemes of the same size.
///
/// **Over GF(p) the two terms need only share a factor up to a scalar.** Since
/// `(αu)⊗v⊗w = u⊗(αv)⊗w`, one term has `p−1` spellings, and matching spellings
/// rather than directions hides most of the flips that exist. `[chen2025]` names
/// this as the reason the walk works well over `Z2` and badly above it, and asks
/// for `Z3`, `Z5` and `Z7`. Comparing directions, and moving the scalar aside
/// just before each flip, is this implementation's answer.
///
/// Measured, so that the answer is not mistaken for a good one: on `f3_3x6` the
/// walk reaches 12 products where `minimise_rank` reaches 10. The quotient makes
/// the method work over GF(p); it does not yet make it competitive there.
///
/// It proves nothing. Every scheme it returns is checked against the map it
/// must compute, by `recovers_map`, never by its own report.
namespace bilinear_rank {

/// One rank-one term, as the three vectors whose tensor product it is.
struct Term {
    std::vector<Element> left;
    std::vector<Element> right;
    std::vector<Element> output;
};

/// A decomposition: the terms sum to the map.
using Scheme = std::vector<Term>;

/// The same object as `⟨L, R, P⟩`, read term by term.
Scheme scheme_of(const Algorithm& algorithm);
Algorithm algorithm_of(const Scheme& scheme);

/// Merge any two terms that share two factors, as many times as possible, and
/// say how many products that removed.
std::size_t merge_shared_terms(const Field& field, Scheme& scheme);

/// What the random flip walk did.
struct FlipReport {
    std::size_t flips = 0;
    std::size_t reductions = 0;
    std::size_t best = 0;
};

/// Flip at random for at most `steps`, reducing whenever possible, and return
/// the smallest scheme seen.
///
/// `seed` is taken rather than drawn, so a run that finds something can be run
/// again and find it again.
Scheme random_flip_walk(const Field& field, Scheme scheme, std::size_t steps, std::uint64_t seed,
            FlipReport* report);

}  // namespace bilinear_rank
