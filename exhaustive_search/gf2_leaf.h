#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "exhaustive_search.h"
#include "gf2_span_basis.h"

/// The leaf test of the exact search, over GF(2), with a bit per entry.
///
/// [`rank_one_basis.h`](rank_one_basis.h) says the leaf is where an exhaustive
/// search spends its life, and instrumenting it agrees: 99% of the search's
/// cycles on `f2_5x5 --target 11` are inside it. This is that same question
/// asked of the same candidates in the same order, over the one field where a
/// span membership test is an exclusive or.
///
/// **It is a case chosen at run time, not a build.** The characteristic comes
/// off the tensor file, so `gf2_leaf_applies` is asked once per search and the
/// general path is what answers when it says no. Nothing over GF(3), GF(5) or
/// the rationals reaches this file.
namespace bilinear_rank {

/// Whether the leaf test has a GF(2) form for this field and shape.
///
/// The width limit is [`gf2_row`](../linear_algebra/gf2_bits.h)'s: the rank-one
/// test reads a row as the low bits of one word. Sixteen columns is the widest
/// shape this repository prices, the 4x4 slices of `<4,4,4>`.
bool gf2_leaf_applies(const Field& field, std::size_t columns);

/// The pool packed into bits once, and the two routes to a rank-one basis.
///
/// Which route to take is not decided here. That rule belongs to
/// [`rank_one_basis.h`](rank_one_basis.h), which owns it for both fields, and
/// duplicating it is exactly how the two would come to disagree about what a
/// leaf is.
///
/// Templated on where the candidates come from, like everything else that walks
/// a pool, so one implementation serves a materialised pool and an addressed
/// one. Instantiated for exactly those two in the source.
template <typename Candidates>
class Gf2Leaf {
   public:
    Gf2Leaf(const Candidates& pool, std::size_t rows, std::size_t columns);

    /// Every pool element tested for membership in `span`, taken greedily so
    /// they stay independent, stopping at `needed` of them.
    ///
    /// `budget` bounds the scan and is the only thing that does; being faster
    /// per element than the general route does not make it finite. It is the
    /// same bound the general route takes, in the same units, so the two cannot
    /// stop at different places.
    std::vector<Matrix> by_scanning_the_pool(const ReducedBasis& span, std::size_t needed,
                                             SearchBudget* budget = nullptr) const;

    /// Every one of the `elements` members of `span`, each tested for rank one.
    std::vector<Matrix> by_walking_the_subspace(const ReducedBasis& span, std::size_t needed,
                                                std::size_t elements,
                                                SearchBudget* budget = nullptr) const;

   private:
    /// Pool element `index` as packed words, from the table when there is one
    /// and into `buffer` when there is not.
    const std::uint64_t* bits_of(std::size_t index, std::vector<std::uint64_t>& buffer) const;

    linear_algebra::Gf2SpanBasis packed(const ReducedBasis& span) const;
    Matrix unpacked(const std::uint64_t* words) const;

    const Candidates& pool_;
    std::size_t rows_;
    std::size_t columns_;
    std::size_t width_;
    std::size_t words_per_map_;
    /// The whole pool, `words_per_map_` words each, end to end. Empty when it
    /// would not fit the memory budget, and then each map is packed on demand.
    std::vector<std::uint64_t> table_;
};

}  // namespace bilinear_rank
