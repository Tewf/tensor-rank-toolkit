#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "measures.h"
#include "tensor_contraction.h"

/// **Rank-sum lower bounds**: what many contractions of one axis can hold
/// *between them*, which is far less than each could hold alone.
///
/// One table, two bounds, and neither dominates the other. Both start from the
/// same observation and both come from `[yang2025]`'s family of pruners; keys are
/// [`../references.md`](../references.md).
///
/// **The one inequality underneath.** Fix an axis `d` and suppose `T` has a
/// rank-`R` decomposition whose factor matrix on that axis is `A`. Contracting by
/// a row vector `v` kills every term `v` annihilates, so
///
/// > `rk(v ·_d T) <= nnz(v A)`
///
/// A contraction rank is therefore a *count of surviving terms*, and both bounds
/// below add that count up over many `v` and divide by how many times each term
/// can be counted.
///
/// **Bound one, along a line** (`line_rank_sum_lower_bound_on_axis`). Take any
/// `v, w` and walk the affine line `{w + s v : s in F}`. Coordinate by
/// coordinate: where `(v A)_r != 0`, the entry `(w A)_r + s (v A)_r` runs through
/// every field element once as `s` does, so it is nonzero for exactly `|F| - 1`
/// of the `|F|` values of `s`; where `(v A)_r == 0` it is nonzero for all `|F|`
/// or for none. Writing `a = nnz(v A)` and `b` for the second group, the total is
/// `a(|F| - 1) + b|F| = |F|(a + b) - a` with `a + b <= R`, so
///
/// > `sum_{s in F} rk((w + s v) ·_d T)  <=  R |F| - rk(v ·_d T)`
///
/// This is the author's `ranksum` pruner. It is an *existential* over pairs, so
/// it fires on whichever pair is worst, and it costs `O(|F|^(2 n_d))`.
///
/// **Bound two, over everything** (`total_rank_sum_lower_bound_on_axis`). Sum
/// over *every* `v` at once instead. Each term's `a_r` is nonzero, so the `v`
/// with `(v A)_r != 0` are the complement of a hyperplane and number exactly
/// `|F|^n_d - |F|^(n_d - 1)`. Exchanging the order of summation,
///
/// > `sum_{v} rk(v ·_d T)  <=  R (|F|^n_d - |F|^(n_d - 1))`
///
/// This is Laskowski's bound, `yang2025thesis` Theorem 3, and the author's Java
/// calls it `lask`. It is one *aggregate* test rather than an existential, and it
/// costs only the table, `O(|F|^n_d)`.
///
/// **Neither dominates, measured.** Over the thirteen fixtures the total bound
/// wins on seven (`gf16` 8 against 6, `f2_3x8` 14 against 12, `cyclic_f2_5` 9
/// against 7), the line bound on one (`matmul_2x2x3` 9 against 8), and five tie.
/// They are
/// independent relaxations of the same inequality, so `rank_sum_lower_bound`
/// takes the maximum and pays for both out of one rank table.
///
/// **Neither dominates the flattening bound either**, and the reason is
/// structural: a flattening is strongest on the *longest* axis, because a
/// flattening with `k` rows has rank at most `k`, while these are strongest on
/// the *shortest*, because their divisor grows exponentially in `n_d` and the
/// numerator only linearly. `f2_5x5` is where that shows: axes 5, 5 and 9, so a
/// contraction rank cannot exceed 5, and the *line* bound stalls at 8 under the
/// flattening's 9. The total bound still reaches 10 there.
/// [`rank_lower_bound.h`](rank_lower_bound.h) is where the three meet.
///
/// **What it costs.** Not polynomial time, unlike a flattening: exponential in
/// the *axis length*, not in the rank. That is fine at these shapes, where the
/// longest axis is 10 over `F2`, and each bound refuses an axis past its own
/// budget. **Refusing an axis, or enumerating only part of one, can only weaken
/// the answer, never invalidate it**, because every bound here is a maximum over
/// independently valid inequalities.
///
/// **Measured by, and only by, this command**, one fixture at a time, because a
/// target below the floor is refused from the bound alone and searches nothing:
///
/// > `decide-rank fixtures/<name>.tensor --target 1`
///
/// One core of an i5-12450H, Release, holding the machine's measurement lock,
/// fastest of three, process start included: `gf16` **3 ms**, `f2_5x5` **9 ms**,
/// `f2_3x8` **30 ms**, and `f3_3x6` **469 ms**, which is the `|F|^(2 n_d)` pair
/// term on a length-8 axis over `F3`. This is the whole bound, both rank sums and
/// the flattening, since `rank_lower_bound` takes a `max` of all three and so
/// evaluates all three.
///
/// The command is written down because the figures before these were not
/// reproducible: three sessions quoted `17/40/688` and then `14/45/718` ms with no
/// record of what produced either, and neither is what the command above returns.
/// They are superseded rather than contradicted, and the way to keep that from
/// recurring is the line above, not a more careful number.
///
/// **So the cheap bound is the one that earns its place.** The line bound is the
/// expensive half and it is decisive on exactly one of the thirteen fixtures,
/// `matmul_2x2x3`, 9 against 8. The total bound wins on seven and ties on five,
/// for `|F|^n_d` instead of `|F|^(2 n_d)`. On `f3_3x6` the 469 ms buys nothing:
/// the total bound's 9 beats the line bound's 8. A caller who wants the floor for
/// almost nothing should pass `work_budget = 0` and keep the table budget, which
/// runs the total bound alone.
namespace linear_algebra {

/// Budget on the pair enumeration of one axis. Generous enough for every fixture
/// here and small enough that no caller waits: `f3_3x6`, the most expensive,
/// costs about 1.7e8 steps.
inline constexpr std::size_t kRankSumWorkBudget = std::size_t(1) << 28;

/// Budget on the rank table itself, which is one contraction and one rank per
/// vector on the axis. The table is what the total bound costs in full.
inline constexpr std::size_t kRankTableBudget = std::size_t(1) << 20;

/// `|F|^length`, the number of vectors on an axis, capped rather than allowed to
/// overflow: anything at the cap is past any budget a caller would set.
inline std::size_t rank_sum_vector_count(std::size_t characteristic, std::size_t length) {
    const std::size_t cap = std::size_t(1) << 40;
    std::size_t count = 1;
    for (std::size_t step = 0; step < length; ++step) {
        if (count > cap / characteristic) return cap;
        count *= characteristic;
    }
    return count;
}

/// Steps the enumeration on one axis costs: one direction per projective point,
/// times every vector, times the work of re-encoding a vector as an index.
inline std::size_t rank_sum_work(std::size_t characteristic, std::size_t length) {
    const std::size_t cap = std::size_t(1) << 40;
    const std::size_t count = rank_sum_vector_count(characteristic, length);
    if (count >= cap || length == 0) return cap;
    const std::size_t directions = (count - 1) / (characteristic - 1);
    if (directions > cap / count) return cap;
    const std::size_t pairs = directions * count;
    if (pairs > cap / length) return cap;
    return pairs * length;
}

/// `rk(v ·_axis T)` for every `v`, indexed by the base-`p` encoding of `v` with
/// position 0 as the least significant digit, the encoding
/// `descent_search/span_enumeration.cpp::coefficient_vector` uses.
template <class Field>
std::vector<std::size_t> contraction_ranks(const Field& field,
                                           const std::vector<MatrixOver<Field>>& slices,
                                           std::size_t axis) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::size_t length = axis_dimension<Field>(slices, axis);
    std::vector<std::size_t> ranks(rank_sum_vector_count(characteristic, length));

    std::vector<typename Field::Element> coefficients(length);
    for (std::size_t index = 0; index < ranks.size(); ++index) {
        std::size_t rest = index;
        for (std::size_t position = 0; position < length; ++position) {
            field.init(coefficients[position], static_cast<int64_t>(rest % characteristic));
            rest /= characteristic;
        }
        ranks[index] = rank(field, contraction(field, slices, axis, coefficients));
    }
    return ranks;
}

/// Laskowski's bound from one axis, given its rank table: the whole table summed,
/// over the number of vectors a single term can survive.
///
/// `yang2025thesis` Theorem 3. Costs a pass over the table and nothing else, which
/// is why it is computed for every axis whose table was affordable at all.
inline std::size_t total_rank_sum_lower_bound_on_axis(const std::vector<std::size_t>& ranks,
                                                     std::size_t characteristic,
                                                     std::size_t length) {
    if (length == 0 || ranks.empty()) return 0;
    // |F|^n - |F|^(n-1): the vectors off the hyperplane a single nonzero term
    // defines, which is how many of the summands that term can appear in.
    std::size_t divisor = characteristic - 1;
    for (std::size_t step = 1; step < length; ++step) divisor *= characteristic;

    std::size_t total = 0;
    for (const std::size_t rank_of_contraction : ranks) total += rank_of_contraction;
    return (total + divisor - 1) / divisor;
}

/// The pair bound from one axis, given its rank table.
///
/// Two savings keep the pair enumeration honest rather than merely smaller. A
/// direction and its nonzero multiples give the *same* inequality, since scaling
/// `v` changes neither `rk(v ·_d T)` nor the line through `w`, so only vectors
/// whose lowest nonzero entry is 1 are used. And the sum ranges over a line, not
/// a point, so each line is walked once rather than once per `w` on it.
inline std::size_t line_rank_sum_lower_bound_on_axis(const std::vector<std::size_t>& ranks,
                                                     std::size_t characteristic,
                                                     std::size_t length) {
    if (length == 0 || ranks.empty()) return 0;
    const std::size_t count = ranks.size();

    // Digits of every index up front, so walking a line costs additions rather
    // than divisions, and the powers needed to encode a vector back.
    std::vector<std::size_t> digits(count * length);
    std::vector<std::size_t> powers(length);
    std::size_t power = 1;
    for (std::size_t position = 0; position < length; ++position) {
        powers[position] = power;
        power *= characteristic;
    }
    for (std::size_t index = 0; index < count; ++index) {
        std::size_t rest = index;
        for (std::size_t position = 0; position < length; ++position) {
            digits[index * length + position] = rest % characteristic;
            rest /= characteristic;
        }
    }

    // `v = 0` degenerates to the statement that no single contraction holds more
    // rank than the whole tensor, which is worth having and costs nothing.
    std::size_t bound = *std::max_element(ranks.begin(), ranks.end());

    std::vector<char> walked(count, 0);
    for (std::size_t direction = 1; direction < count; ++direction) {
        std::size_t lowest = 0;
        while (digits[direction * length + lowest] == 0) ++lowest;
        if (digits[direction * length + lowest] != 1) continue;

        std::fill(walked.begin(), walked.end(), char(0));
        for (std::size_t start = 0; start < count; ++start) {
            if (walked[start] != 0) continue;
            std::size_t total = 0;
            std::size_t index = start;
            for (std::size_t step = 0; step < characteristic; ++step) {
                walked[index] = 1;
                total += ranks[index];
                std::size_t next = 0;
                for (std::size_t position = 0; position < length; ++position) {
                    const std::size_t sum = digits[index * length + position] +
                                            digits[direction * length + position];
                    next += (sum % characteristic) * powers[position];
                }
                index = next;
            }
            bound = std::max(bound, (total + ranks[direction] + characteristic - 1) /
                                        characteristic);
        }
    }
    return bound;
}

/// The largest of both bounds over all three axes, one rank table per axis.
///
/// The table is the shared cost, so an axis is either affordable for both bounds
/// or for neither; the pair bound has its own, tighter budget on top, since it is
/// quadratic in a table the total bound only reads once.
template <class Field>
std::size_t rank_sum_lower_bound(const Field& field,
                                 const std::vector<MatrixOver<Field>>& slices,
                                 std::size_t work_budget = kRankSumWorkBudget,
                                 std::size_t table_budget = kRankTableBudget) {
    if (slices.empty()) return 0;
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    std::size_t bound = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const std::size_t length = axis_dimension<Field>(slices, axis);
        if (rank_sum_vector_count(characteristic, length) > table_budget) continue;

        const std::vector<std::size_t> ranks = contraction_ranks(field, slices, axis);
        bound = std::max(bound,
                         total_rank_sum_lower_bound_on_axis(ranks, characteristic, length));
        if (rank_sum_work(characteristic, length) > work_budget) continue;
        bound = std::max(bound,
                         line_rank_sum_lower_bound_on_axis(ranks, characteristic, length));
    }
    return bound;
}

}  // namespace linear_algebra
