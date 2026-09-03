/// The two rank-sum bounds, checked for soundness, for strength, and against a
/// second implementation of themselves.
///
/// A lower bound is the one error class nothing downstream catches: a rank the
/// bound wrongly forbids is never searched, and the answer comes back confidently
/// wrong. So this asserts three separate things. **Sound**: on every fixture whose
/// rank is known from above, neither bound exceeds that rank. **Strong**: each is
/// pinned per fixture, so a change in either is visible, and the two are shown not
/// to dominate each other. **Correct as written**: the line bound's fast
/// enumeration, which uses one direction per projective point and walks each line
/// once, agrees with the obvious double loop over every ordered pair of vectors.
/// That last check is the pattern `methods/bilinear_rank/orbit_reduction/group_construction.h` already
/// uses, where a brute-force enumeration and a closed form must agree on small
/// shapes.
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "check.h"
#include "rank_lower_bound.h"
#include "tensor_contraction.h"
#include "tensor_file.h"
#include "tensor_flattening.h"
#include "tensor_rank_sum.h"

namespace {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

struct Fixture {
    const char* name;
    /// A rank that is known to be achievable, so no bound may exceed it.
    long long known_rank;
    /// `[yang2025]`'s pruner: an existential over pairs of vectors.
    long long line;
    /// Laskowski, `yang2025thesis` Theorem 3: one aggregate over every vector.
    long long total;
};

/// Ranks from `famous_tensors/`, `methods/bilinear_rank/greedy_heuristic/known_ranks.md` and the
/// fixtures' own table. Where a rank is not settled, the column holds the best
/// decomposition anyone here has actually exhibited, which is still an upper
/// bound a lower bound may not cross.
///
/// **Neither bound dominates.** The total bound wins on seven of the thirteen
/// fixtures below, by as much as `gf16`'s 8 against 6 and `f2_3x8`'s 14 against
/// 12; the line bound wins on `matmul_2x2x3`, 9 against 8; five tie. That is why
/// `rank_sum_lower_bound` computes both out of one rank table instead of picking
/// one.
constexpr Fixture kFixtures[] = {
    {"cyclic_f2_5", 10, 7, 9},
    {"f2_2x2", 3, 3, 3},
    {"f2_2x3", 5, 5, 5},
    {"f2_3x8", 15, 12, 14},
    {"f2_4x7", 16, 11, 14},
    {"f2_5x5", 13, 8, 10},
    {"f3_3x6", 10, 8, 9},
    {"gf16_multiplication", 9, 6, 8},
    {"gf4_multiplication", 3, 3, 3},
    {"gf8_multiplication", 6, 5, 6},
    {"matmul_2x2x2", 7, 6, 6},
    {"matmul_2x2x3", 11, 9, 8},
    {"w_state", 3, 3, 3},
};

/// The line bound written the obvious way: every ordered pair of vectors, no
/// projective saving, no coset saving, no precomputed digits, ranks recomputed
/// rather than tabulated. Slow, and deliberately shares no code with the
/// implementation it checks beyond `contraction` itself.
std::size_t brute_force_axis_bound(const ModularField& field,
                                   const std::vector<ModularMatrix>& slices, std::size_t axis) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::size_t length = linear_algebra::axis_dimension<ModularField>(slices, axis);
    std::size_t count = 1;
    for (std::size_t step = 0; step < length; ++step) count *= characteristic;

    const auto vector_at = [&](std::size_t index) {
        std::vector<int64_t> entries(length);
        for (std::size_t position = 0; position < length; ++position) {
            entries[position] = static_cast<int64_t>(index % characteristic);
            index /= characteristic;
        }
        return entries;
    };
    const auto rank_of = [&](const std::vector<int64_t>& entries) {
        return linear_algebra::rank(field,
                                    linear_algebra::contraction(field, slices, axis, entries));
    };

    std::size_t bound = 0;
    for (std::size_t direction = 0; direction < count; ++direction) {
        const std::vector<int64_t> v = vector_at(direction);
        const std::size_t rank_v = rank_of(v);
        for (std::size_t offset = 0; offset < count; ++offset) {
            const std::vector<int64_t> w = vector_at(offset);
            std::size_t total = 0;
            for (std::size_t scalar = 0; scalar < characteristic; ++scalar) {
                std::vector<int64_t> point(length);
                for (std::size_t position = 0; position < length; ++position) {
                    point[position] = static_cast<int64_t>(
                        (static_cast<std::size_t>(w[position]) +
                         scalar * static_cast<std::size_t>(v[position])) %
                        characteristic);
                }
                total += rank_of(point);
            }
            bound = std::max(bound, (total + rank_v + characteristic - 1) / characteristic);
        }
    }
    return bound;
}

/// The best each bound gives over the three axes, out of one table per axis.
struct Bounds {
    std::size_t line = 0;
    std::size_t total = 0;
};

Bounds bounds_of(const ModularField& field, const std::vector<ModularMatrix>& slices) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    Bounds best;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const std::size_t length = linear_algebra::axis_dimension<ModularField>(slices, axis);
        const std::vector<std::size_t> ranks =
            linear_algebra::contraction_ranks(field, slices, axis);
        best.line = std::max(best.line, linear_algebra::line_rank_sum_lower_bound_on_axis(
                                            ranks, characteristic, length));
        best.total = std::max(best.total, linear_algebra::total_rank_sum_lower_bound_on_axis(
                                              ranks, characteristic, length));
    }
    return best;
}

/// The tensor of a single rank-one map, whose every contraction rank is 0 or 1.
formats::Tensor rank_one_tensor() {
    formats::Tensor tensor;
    tensor.characteristic = 2;
    // T(row, column, slice) = u[row] v[column] w[slice], with u = v = w = (1,1).
    for (std::size_t slice = 0; slice < 2; ++slice) {
        ModularMatrix matrix(2, 2);
        for (std::size_t row = 0; row < 2; ++row) {
            for (std::size_t column = 0; column < 2; ++column) matrix(row, column) = 1;
        }
        tensor.slices.push_back(matrix);
    }
    return tensor;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    bool line_ever_wins = false;
    bool total_ever_wins = false;

    for (const Fixture& fixture : kFixtures) {
        const auto tensor = formats::read_tensor_file(fixtures + "/" +
                                                            std::string(fixture.name) + ".tensor");
        const ModularField field(tensor.characteristic);
        const std::string name = fixture.name;

        const std::size_t flattening =
            linear_algebra::flattening_lower_bound(field, tensor.slices);
        const Bounds measured = bounds_of(field, tensor.slices);
        const std::size_t together = std::max(measured.line, measured.total);

        // Sound. This is the check that matters: a bound above a rank someone has
        // exhibited means the implementation is wrong, not that the rank is.
        check::equal(name + " neither bound exceeds a known rank",
                     static_cast<long long>(together) <= fixture.known_rank ? 1 : 0, 1);
        check::equal(name + " line bound", static_cast<long long>(measured.line), fixture.line);
        check::equal(name + " total bound", static_cast<long long>(measured.total), fixture.total);

        // The shared entry point agrees with computing the two separately.
        check::equal(name + " rank sum bound",
                     static_cast<long long>(
                         linear_algebra::rank_sum_lower_bound(field, tensor.slices)),
                     static_cast<long long>(together));

        // And the combined bound is at least the largest of the three here.
        //
        // It was an equality until Griesmer joined `rank_lower_bound` as a fourth
        // term, and on `f2_5x5` that term wins, 12 against 10. Asserting equality
        // here would either pin this file to a bound it does not compute or make
        // it a second copy of `rank_metric_bound`'s own test. What belongs here is
        // that the shared entry point never loses to the rank sums, and the exact
        // value of every term is asserted where that term lives.
        check::equal(name + " combined bound is at least the rank sums",
                     linear_algebra::rank_lower_bound(field, tensor.slices) >=
                             std::max(together, flattening)
                         ? 1
                         : 0,
                     1);

        if (measured.line > measured.total) line_ever_wins = true;
        if (measured.total > measured.line) total_ever_wins = true;
    }

    // Both directions occur, so neither bound may be dropped for the other.
    check::equal("the line bound wins somewhere", line_ever_wins ? 1 : 0, 1);
    check::equal("the total bound wins somewhere", total_ever_wins ? 1 : 0, 1);

    // The line bound's fast enumeration against the obvious one, on every axis
    // short enough to enumerate twice. This is what would catch a wrong canonical
    // representative or a line walked the wrong number of times.
    for (const Fixture& fixture : kFixtures) {
        const auto tensor = formats::read_tensor_file(fixtures + "/" +
                                                            std::string(fixture.name) + ".tensor");
        const ModularField field(tensor.characteristic);
        const auto characteristic = static_cast<std::size_t>(tensor.characteristic);
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const std::size_t length =
                linear_algebra::axis_dimension<ModularField>(tensor.slices, axis);
            if (linear_algebra::rank_sum_vector_count(characteristic, length) > 64) continue;
            const std::vector<std::size_t> ranks =
                linear_algebra::contraction_ranks(field, tensor.slices, axis);
            check::equal(std::string(fixture.name) + " axis " + std::to_string(axis) +
                             " agrees with brute force",
                         static_cast<long long>(linear_algebra::line_rank_sum_lower_bound_on_axis(
                             ranks, characteristic, length)),
                         static_cast<long long>(
                             brute_force_axis_bound(field, tensor.slices, axis)));
        }
    }

    // A rank-one tensor: both bounds must say 1, not 0 and not 2.
    {
        const auto tensor = rank_one_tensor();
        const ModularField field(tensor.characteristic);
        check::equal("a rank-one tensor bounds at one",
                     static_cast<long long>(
                         linear_algebra::rank_sum_lower_bound(field, tensor.slices)),
                     1);
    }

    // An axis past a budget is skipped, not enumerated, and skipping only weakens:
    // with no table budget at all the answer falls back to zero rather than
    // hanging or lying.
    {
        const auto tensor = formats::read_tensor_file(fixtures + "/matmul_2x2x2.tensor");
        const ModularField field(tensor.characteristic);
        check::equal("no table budget skips every axis",
                     static_cast<long long>(
                         linear_algebra::rank_sum_lower_bound(field, tensor.slices, 0, 0)),
                     0);
        // With the table affordable but no room for pairs, the total bound still
        // answers, which is the point of separating the two budgets.
        check::equal("the total bound survives a zero pair budget",
                     static_cast<long long>(linear_algebra::rank_sum_lower_bound(
                         field, tensor.slices, 0, linear_algebra::kRankTableBudget)),
                     6);
    }

    return check::report("tensor rank sum");
}
