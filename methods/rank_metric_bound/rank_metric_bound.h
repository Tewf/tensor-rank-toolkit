#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "tensor_contraction.h"
#include "tensor_rank_sum.h"

/// What the two rank-metric parameters of a slice space force on their own.
///
/// Read a slice space `S` of a tensor as a **rank-metric code**: `k = dim S`, and
/// `d = min{rk(M) : M in S, M != 0}` is its minimum rank distance. Two lower
/// bounds on tensor rank follow from `(k, d)` alone and from nothing else about
/// the tensor.
///
/// **Kruskal's bound**, `rank(T) >= k + d - 1`. In this notation it is
/// `[byrne2021, Thm. 5.4]`, `trk(C) >= dim_Fq(C) + d(C) - 1`. Their code `C` is a
/// slice space, their Def. 2.1 being the span of the slices; their `k` is that
/// space's dimension and their `d` its minimum rank distance, both fixed by their
/// Def. 5.3. So the two symbols mean here exactly what they mean there. Key in
/// [`../../references.md`](../../references.md).
///
/// **Whose bound it is, stated as carefully as it was checked.** Byrne and
/// Cotardo attribute it to J. B. Kruskal, *Three-way arrays: rank and uniqueness
/// of trilinear decompositions*, Linear Algebra Appl. **18** (1977), 95-138,
/// **Corollary 1**, and `[bnrs2019]`'s Theorem 3.6 cites the same paper and the
/// same corollary for the same inequality; both bibliographies were checked, not
/// just their prose. **That original was not read**: it is behind a paywall with
/// no repository copy, so the attribution here rests on two independent papers
/// agreeing and not on Kruskal. What *was* read is the coding-theoretic form and
/// its own proof.
///
/// **The Griesmer form**, which is never weaker and costs one more loop.
/// Write a rank-`R` decomposition `T = sum_r a_r (x) b_r (x) c_r` and send a
/// vector `v` to its coefficient word `(<v, a_1>, ..., <v, a_R>)`. Contracting
/// kills the terms `v` annihilates, so `v ·_d T` is a sum of `wt(word)` rank-one
/// matrices and `rk(v ·_d T) <= wt(word)`. Restrict that map to any complement of
/// the kernel `{v : v ·_d T = 0}` and it becomes injective, because a `v` whose
/// word is zero has a zero contraction; the image is then a linear block code of
/// length `R`, dimension exactly `k`, and minimum Hamming weight at least `d`.
/// That is `[bnrs2019, Cor. 4.14(2)]`, and the key is now in
/// [`../../references.md`](../../references.md). Any linear `[R, k, >= d]` code obeys
/// Griesmer, so
///
/// > `rank(T) >= sum_{j=0}^{k-1} ceil(d / |F|^j)`
///
/// **Griesmer's name is in neither paper.** `[bnrs2019, Cor. 4.14(2)]` hands over
/// the block code and stops there, and `[byrne2021]` never mentions him either.
/// Applying Griesmer to that code is this repository's step, which is why the
/// argument is written out above instead of cited to a number.
///
/// **Why nothing here tests for conciseness first.** `[bnrs2019, Cor. 4.14]` is
/// stated for `C = V<D>W^T` with `V` and `W` of full ranks `n` and `m`, that is,
/// for a tensor concise on the two surviving axes, and `[byrne2021]`'s Kruskal
/// form inherits the `1`-nondegeneracy of Kruskal's original. The argument above
/// asks for neither: it reads `k` and `d` off the slice space itself, and an axis
/// longer than its slice space only means a bigger kernel to take a complement
/// of. The test pins that with a two-slice tensor whose slices are equal.
///
/// Kruskal's bound is the Singleton relaxation of the same statement, since every
/// term past the first is at least 1. They part company exactly when `d > |F|`,
/// which over `GF(2)` is almost always.
///
/// **Both hold over a finite field**, which is the one thing worth checking
/// before importing a bound stated for the reals: `[bnrs2019]`'s section 3 opens
/// "F denotes an arbitrary field", and `[byrne2021]`'s section 5 is stated over
/// `F_q` throughout, `C <= F_q^(n x m)`. Nothing in either argument asks the
/// field to be infinite, ordered or closed. `[bnrs2019, Cor. 4.15]` is a proof of
/// the Kruskal form that never leaves finite fields, so this does not rest on
/// reading a 1977 paper about the reals correctly.
///
/// **`|F|` below is the characteristic**, and that is the same number only
/// because [`field.h`](../../core/linear_algebra/field.h)'s `is_prime` restricts every
/// rank claim in this repository to `GF(p)`. Griesmer's base is the *cardinality*:
/// over `GF(4)` the code below would count `k` in `F_2` steps and divide by 2,
/// which returns a number **larger** than Griesmer permits, and so a false
/// refutation. `contraction_ranks` already assumes the same thing, enumerating
/// base-`p` digits over an axis, so an extension field has to be handled in both
/// places at once or in neither.
///
/// **The equivalence underneath both** is that a rank-`R` decomposition of `T` is
/// exactly a set of `R` independent rank-one matrices whose span contains `S`:
/// `[byrne2021, Lem. 2.7]`, Bürgisser-Clausen-Shokrollahi Prop. 14.45, the same
/// statement [`../canonical_factorisation/`](../canonical_factorisation/README.md)
/// is built on and
/// [`../bilinear_rank/exhaustive/`](../bilinear_rank/exhaustive/README.md) searches.
///
/// **What they cost.** `d` is a minimum over the whole slice space, so it costs
/// the `|F|^n_d` contraction ranks of one axis: the table
/// [`tensor_rank_sum.h`](../../core/linear_algebra/tensor_rank_sum.h) already builds for
/// Laskowski's bound and re-reads for the line bound. Given that table both
/// bounds here are one linear scan, which is why the per-axis functions take the
/// table rather than the tensor. A caller already paying for the rank sums pays
/// nothing more.
///
/// **What they are worth here**:
/// [`what-each-is-worth.md`](what-each-is-worth.md), which has the table on every
/// fixture. Kruskal's bound never beats what this repository already had; the
/// Griesmer form does, once, and [`joining-the-shared-floor.md`](joining-the-shared-floor.md) is
/// what it would take for a caller to get that for free.
namespace rank_metric_bound {

/// `d`: the least rank of a nonzero element of the slice space, given that axis's
/// contraction-rank table. Zero when the slice space is zero, which is the one
/// case where neither bound below applies.
///
/// A vector outside the kernel is exactly a vector whose contraction is nonzero,
/// so zero entries are skipped rather than only the entry at index 0. That
/// matters for a tensor that is not concise, where the kernel is bigger than
/// `{0}`.
inline std::size_t minimum_rank_distance(const std::vector<std::size_t>& ranks) {
    std::size_t distance = 0;
    for (const std::size_t rank_of_contraction : ranks) {
        if (rank_of_contraction == 0) continue;
        if (distance == 0 || rank_of_contraction < distance) distance = rank_of_contraction;
    }
    return distance;
}

/// `k`: the dimension of the slice space, read off the same table.
///
/// `v -> v ·_d T` is linear, so the vectors it kills form a subspace, and a
/// subspace of `F^n` of dimension `n - k` has exactly `|F|^(n-k)` elements.
/// Counting the zeros of the table therefore gives `k` with no second pass over
/// the tensor. `linear_algebra::flattening_ranks` computes the same number by
/// elimination, and the test requires the two to agree on every fixture.
inline std::size_t slice_space_dimension(const std::vector<std::size_t>& ranks,
                                         std::size_t characteristic) {
    if (ranks.empty() || characteristic < 2) return 0;
    std::size_t kernel_size = 0;
    for (const std::size_t rank_of_contraction : ranks) {
        if (rank_of_contraction == 0) ++kernel_size;
    }
    // Both counts are powers of the characteristic, so dividing one down to the
    // other counts `k` directly and needs no logarithm.
    std::size_t dimension = 0;
    for (std::size_t quotient = ranks.size(); quotient > kernel_size; quotient /= characteristic) {
        ++dimension;
    }
    return dimension;
}

/// `sum_j ceil(d / |F|^j)` from one axis, the Griesmer length of the block code
/// the decomposition induces. Never smaller than Kruskal's `k + d - 1`, since
/// the first term is `d` and the other `k - 1` are at least 1 each.
///
/// The division walks `d` down rather than `|F|^j` up, so nothing overflows at
/// any `k`, and it stops once the terms are all 1 and the rest is a count.
inline std::size_t griesmer_bound_on_axis(const std::vector<std::size_t>& ranks,
                                          std::size_t characteristic) {
    const std::size_t distance = minimum_rank_distance(ranks);
    if (distance == 0) return 0;
    const std::size_t dimension = slice_space_dimension(ranks, characteristic);

    std::size_t total = 0;
    std::size_t term = distance;
    for (std::size_t index = 0; index < dimension; ++index) {
        if (term <= 1) return total + (dimension - index);
        total += term;
        term = (term + characteristic - 1) / characteristic;
    }
    return total;
}

/// The best a per-axis bound gives over the three axes, one contraction-rank
/// table each.
///
/// An axis whose table is past `table_budget` is skipped rather than enumerated,
/// which can only weaken the answer and never invalidate it, exactly as
/// `rank_sum_lower_bound` skips one. The budget is shared with that function on
/// purpose: the two want the same table, so an axis affordable for one is
/// affordable for the other.
template <class Field, class BoundOnAxis>
std::size_t best_over_axes(const Field& field,
                           const std::vector<linear_algebra::MatrixOver<Field>>& slices,
                           BoundOnAxis bound_on_axis, std::size_t table_budget) {
    if (slices.empty()) return 0;
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    std::size_t best = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const std::size_t length = linear_algebra::axis_dimension<Field>(slices, axis);
        if (linear_algebra::rank_sum_vector_count(characteristic, length) > table_budget) continue;
        const std::vector<std::size_t> ranks =
            linear_algebra::contraction_ranks(field, slices, axis);
        best = std::max(best, bound_on_axis(ranks, characteristic));
    }
    return best;
}

/// The Griesmer form, over the best axis, and the only one wired in.
///
/// Kruskal's `k + d - 1` used to sit beside it and is retired to
/// `rejected-experiments`: Griesmer is at least as large on every fixture here
/// and strictly larger on several, so it is dominated on its own field.
/// [`what-each-is-worth.md`](what-each-is-worth.md) keeps both columns, because
/// the evidence for a removal outlives the code.
template <class Field>
std::size_t griesmer_lower_bound(const Field& field,
                                 const std::vector<linear_algebra::MatrixOver<Field>>& slices,
                                 std::size_t table_budget = linear_algebra::kRankTableBudget) {
    return best_over_axes(field, slices, griesmer_bound_on_axis, table_budget);
}

}  // namespace rank_metric_bound
