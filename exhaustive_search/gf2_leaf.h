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
///
/// ## What it is worth, measured
///
/// Measured 2026-08-19 under [`../MEASURING.md`](../MEASURING.md): one core,
/// fastest of three, on a machine whose throttle counters did not move across
/// the runs. The number taken is the search's own `N nodes in S s`, which
/// excludes process start-up and pool construction, so what is left is the tree
/// the two routes differ inside. Every pair below is **one question asked
/// twice**, `--general-leaf` selecting the path this file replaces:
///
///     build/exhaustive_search/decide-rank FIXTURE --target K [--node-limit N]
///     build/exhaustive_search/decide-rank FIXTURE --target K [--node-limit N] --general-leaf
///
/// | question | route | nodes | general | GF(2) | factor |
/// |---|---|---|---|---|---|
/// | `gf16_multiplication --target 8 --node-limit 200000` | scan, 225 | 200 000 | 4.47 s | 0.750 s | **6.0x** |
/// | `f2_3x8 --target 14 --node-limit 20000` | scan, 1 785 | 20 000 | 8.27 s | 0.995 s | **8.3x** |
/// | `f2_5x5 --target 10` | scan, 961 | 959 | 0.149 s | 0.0148 s | **10.1x** |
/// | `f2_5x5 --target 11` | scan, 961 | 459 239 | 77.88 s | 7.69 s | **10.1x** |
/// | `matmul_2x2x2 --target 6` | walk, 64 | 25 399 | 0.560 s | 0.0347 s | **16.2x** |
/// | `matmul_3x3x3 --target 23 --node-limit 300` | scan, 261 121 | 300 | 82.28 s | 3.55 s | **23.2x** |
/// | `matmul_3x3x3 --target 17 --node-limit 60` | walk, 131 072 | 60 | 16.66 s | 0.420 s | **39.6x** |
///
/// **6.0x to 39.6x, and the two routes are not the same story.** Scanning the
/// pool gains 6x to 23x, rising with the width of a map because the width is
/// what the XOR replaces: a `Θ(d·w)` reduction through Givaro becomes
/// `Θ(d·w/64)` word operations, and 4x4 is a quarter of a word where 9x9 is one
/// and a bit. Walking the subspace gains more, and the cleanest evidence is the
/// two 4x4 rows: **6.0x scanning against 16.2x walking, at one shape**. There
/// the win is not the width at all. `gf2_is_rank_one` is one comparison per row
/// where [`rank`](../linear_algebra/measures.h) is a Gaussian elimination, so
/// those two differ by a factor of the shape and not by a factor of 64.
///
/// **The general column moved on 2026-08-20 and these ratios are now upper
/// bounds.** `is_rank_one` replaced `rank(...) != 1` on that path, so the column
/// this file is measured *against* got faster. Re-run on `matmul_2x2x2
/// --target 6`, the general leaf is **0.1305 s against the 0.560 s above** — and
/// that re-run was taken under load, which can only slow it, so the quiet figure
/// is no larger. **That row's 16.2x is therefore at most 3.8x.** The other rows
/// have not been re-taken. Nothing here is wrong about the day it was measured;
/// it is wrong about today, which is the failure this repository already had once
/// with the card's baseline.
///
/// **Where it loses.** Packing the pool is a one-off cost the general path
/// never pays, and a search that reaches no leaf at all pays it for nothing: at
/// `<3,3,3>` the 261 121-map table costs **40 ms**, against 8.6 us for the same
/// run on the general path, so `--node-limit 5` measures this file at 4 700x
/// *slower*. It is repaid by the first leaf. The first leaf on that question is
/// node 15, where the factor is already 5.8x, and by node 40 it is 20.7x.
/// Nothing published here stops before its first leaf, so no measurement in
/// this repository is on the losing side of that line.
///
/// The 10.1x on `f2_5x5 --target 11` also puts a floor under the claim above
/// that the leaf is where the search lives: by Amdahl, a whole-run factor of
/// 10.1 needs the replaced part to have been **at least 90%** of the general
/// run, whatever the leaf itself gained.
namespace bilinear_rank {

/// Whether the leaf test has a GF(2) form for this field and shape.
///
/// The width limit is [`gf2_row`](../linear_algebra/gf2_bits.h)'s: the rank-one
/// test reads a row as the low bits of one word. Sixteen columns is the widest
/// shape this repository prices, the 4x4 slices of `<4,4,4>`.
///
/// Answers false for every field and shape while the leaf is switched off
/// below, so one switch turns the whole file off rather than each caller
/// remembering to ask twice.
bool gf2_leaf_applies(const Field& field, std::size_t columns);

/// Whether this file is offered at all. True unless a run asks otherwise.
///
/// **It exists so the two routes can be timed on the same question.** A leaf
/// that is only ever taken cannot be compared with the one it replaced, and a
/// comparison across two different questions is not a comparison. So
/// `--general-leaf` on `decide-rank` sets this false for the run, the general
/// path answers every leaf, the node count is unchanged because only the leaf
/// test differs, and the two wall clocks are of the same tree.
///
/// It is a process-wide setting like `set_worker_count` and
/// `set_memory_budget`, read once per search rather than per leaf, and nothing
/// but a command line writes it.
void set_gf2_leaf_offered(bool offered);
bool gf2_leaf_offered();

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
    Gf2Leaf(const Field& field, const Candidates& pool, std::size_t rows, std::size_t columns);

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
    ///
    /// **Where the table does not fit, the outer product is formed in bits.**
    /// `at(i)` builds a `Matrix` and multiplies `rows * columns` field elements
    /// through Givaro, and `gf2_pack` then reads all of them back one at a time;
    /// over GF(2) both are a detour. Element `i` is `lefts[i / rc] (x)
    /// rights[i % rc]`, the layout is row major, so row `r` of the product is
    /// the right vector where the left vector has a bit and zero where it does
    /// not: one shift and one or per set bit. Measured at 129.1 ns an element
    /// against 940.2 for the route above, on the 16x16 slices of `<4,4,4>` where
    /// the table is 137 GiB and is never built.
    const std::uint64_t* bits_of(std::size_t index, std::vector<std::uint64_t>& buffer) const;

    /// The two vector lists as bit patterns, in the order `at(i)` indexes them.
    /// Empty when the table exists, since nothing then asks, and when the pool
    /// handed over is not the grid those lists generate.
    std::vector<std::uint64_t> left_masks_;
    std::vector<std::uint64_t> right_masks_;
    std::size_t right_count_ = 0;

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
