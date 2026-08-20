/// The two bounds a slice space's `(k, d)` forces: sound, cheap, and one of them
/// worth having.
///
/// Five things are asserted. **Sound**: neither bound exceeds a rank somebody has
/// exhibited, which is the one error class nothing downstream catches, since a
/// rank a bound wrongly forbids is never searched. That runs on every fixture and
/// then on 120 random tensors built as a sum of a known number of rank-one terms,
/// because twenty-four fixtures reach twenty-four slice-space shapes and the shapes
/// that would break the argument are the awkward ones: dependent terms, and an axis
/// longer than its slice space. **Pinned**: the value of each on each fixture, and
/// the floor `max(rank_lower_bound, Griesmer)` that wiring this in would produce,
/// so the one fixture where that differs from `rank_lower_bound` today is a
/// checked number rather than a claim in a README.
/// **Ordered**: Griesmer is never below Kruskal, which is arithmetic and not a
/// measurement, and Kruskal is never below the flattening rank of the axis it
/// came from, because `k` *is* that rank and `d >= 1`. **Read off the right
/// space**: `k` taken from the contraction table must equal the flattening rank
/// computed by elimination, which is the check that would catch the bound
/// reading an axis where it means a slice space.
///
/// That Kruskal's bound is *weaker* than `rank_lower_bound` is measured and
/// recorded in [`../what-each-is-worth.md`](../what-each-is-worth.md), not
/// asserted, following the rule
/// `pencil_rank/tests/test_projection_lower_bound.cpp` states: a test pinning a
/// bound as weaker fails the day somebody strengthens the bound.
///
/// The elapsed time per fixture is printed and never asserted. It is what
/// [`../what-each-is-worth.md`](../what-each-is-worth.md)'s cost column was
/// measured with, under
/// [`../../MEASURING.md`](../../MEASURING.md), and CI reads it as noise.
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "check.h"
#include "rank_lower_bound.h"
#include "rank_metric_bound.h"
#include "tensor_contraction.h"
#include "tensor_file.h"
#include "tensor_flattening.h"
#include "timing.h"

namespace {

struct Fixture {
    const char* name;
    /// A rank that is known to be achievable, so no lower bound may exceed it.
    /// From `descent_search/known_ranks.md`, `famous_tensors/`,
    /// `pencil_rank/what-the-literature-settles.md` and the fixtures' own table.
    ///
    /// **Two of the rows below hold a naive count instead**, and the difference
    /// matters here more than anywhere: this column is the one thing that would
    /// catch a bound overshooting a real rank, so a number in it has to be a
    /// decomposition somebody can produce. `gf32` and `gf64` are published at 13
    /// and 15, and neither figure was traced to a table anyone here has read, so
    /// they sit in `fixtures/published-targets.md` as targets and this column
    /// holds 25 and 36, which is the naive algorithm the fixture itself
    /// exhibits. Sound and weak beats tight and unsourced.
    long long known_rank;
    /// `k + d - 1`, the best over the three axes.
    /// `sum_j ceil(d / p^j)`, the best over the three axes.
    long long griesmer;
    /// `max(rank_lower_bound, Griesmer)`: the floor a caller would get if
    /// [`../joining-the-shared-floor.md`](../joining-the-shared-floor.md)'s one-line edit to
    /// `linear_algebra/rank_lower_bound.h` were made. Pinned here because it is
    /// this module's whole claim, and because whoever makes that edit needs the
    /// twenty-four numbers it has to produce.
    long long combined_floor;
};

constexpr Fixture kFixtures[] = {
    {"cyclic_f2_5", 10, 5, 9},
    {"cyclic_f2_7", 13, 7, 12},
    {"f2_2x2", 3, 3, 3},
    {"f2_2x3", 5, 5, 5},
    {"f2_3x8", 15, 14, 14},
    {"f2_4x7", 16, 14, 14},
    {"f2_5x5", 13, 12, 12},
    {"f3_3x6", 10, 9, 9},
    {"gf16_multiplication", 9, 8, 8},
    {"gf32_multiplication", 25, 12, 12},
    {"gf4_multiplication", 3, 3, 3},
    {"gf64_multiplication", 36, 14, 14},
    {"gf8_multiplication", 6, 6, 6},
    {"matmul_2x2x2", 7, 5, 6},
    {"matmul_2x2x3", 11, 7, 9},
    {"matmul_2x3x4", 20, 13, 14},
    {"matmul_3x3x3", 23, 12, 14},
    {"matmul_3x3x4", 29, 15, 18},
    {"matmul_3x4x4", 38, 19, 21},
    {"pencil_irreducible_f2_4", 6, 6, 6},
    {"pencil_nilpotent_f2_3", 4, 3, 4},
    {"pencil_singular_f2_2x3", 3, 3, 3},
    {"pencil_split_f3_3", 3, 3, 3},
    {"w_state", 3, 2, 3},
};

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;
/// A two-slice tensor whose slices are equal, so `v -> v ·_2 T` has a kernel and
/// the slice space is smaller than its axis. Its rank is 1, and a bound that
/// counted the axis instead of the space would say 2.
linear_algebra::Tensor repeated_slice_tensor() {
    linear_algebra::Tensor tensor;
    tensor.characteristic = 2;
    for (std::size_t slice = 0; slice < 2; ++slice) {
        ModularMatrix matrix(2, 2);
        matrix(0, 0) = 1;
        tensor.slices.push_back(matrix);
    }
    return tensor;
}
/// A sum of `terms` random rank-one terms, so the rank is at most `terms` however
/// the coefficients fall, and any lower bound above `terms` is a false refutation.
///
/// The same construction as `linear_algebra/tests/test_flattening.cpp`'s
/// `low_rank_tensor`, written again rather than shared: it lives in a test
/// translation unit, and making one test suite link another's objects to reach a
/// fixture builder would couple them for the sake of twenty lines.
std::vector<ModularMatrix> low_rank_tensor(const ModularField& field, int64_t characteristic,
                                          std::mt19937& generator, std::size_t rows,
                                          std::size_t columns, std::size_t depth,
                                          std::size_t terms) {
    std::vector<ModularMatrix> slices(depth, ModularMatrix(rows, columns));
    std::uniform_int_distribution<int64_t> entries(0, characteristic - 1);
    for (std::size_t term = 0; term < terms; ++term) {
        std::vector<int64_t> left(rows), right(columns), coefficient(depth);
        for (int64_t& entry : left) entry = entries(generator);
        for (int64_t& entry : right) entry = entries(generator);
        for (int64_t& entry : coefficient) entry = entries(generator);
        for (std::size_t slice = 0; slice < depth; ++slice) {
            for (std::size_t row = 0; row < rows; ++row) {
                for (std::size_t column = 0; column < columns; ++column) {
                    int64_t product = 0;
                    field.mul(product, left[row], right[column]);
                    field.mulin(product, coefficient[slice]);
                    field.addin(slices[slice](row, column), product);
                }
            }
        }
    }
    return slices;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string directory = argc > 1 ? argv[1] : "fixtures";

    for (const Fixture& fixture : kFixtures) {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/" + fixture.name + ".tensor");
        const ModularField field(tensor.characteristic);
        const std::string label = fixture.name;
        const auto characteristic = static_cast<std::size_t>(tensor.characteristic);

        const cli::Clock::time_point started = cli::Clock::now();
        const long long griesmer = static_cast<long long>(
            rank_metric_bound::griesmer_lower_bound(field, tensor.slices));
        const double seconds = cli::elapsed_seconds(started);
        check::equal(label + " Griesmer bound", griesmer, fixture.griesmer);
        check::equal(label + " does not exceed a known rank",
                     griesmer <= fixture.known_rank ? 1 : 0, 1);
        // Per axis: `k` read off the contraction table must be the flattening
        // rank, and `k + d - 1` must reach at least it.
        const std::vector<std::size_t> flattenings =
            linear_algebra::flattening_ranks(field, tensor.slices);
        // The `(k, d)` of the axis the answer came from. Printed and never
        // asserted: it is what ../what-each-is-worth.md's reading of where each bound wins
        // and loses rests on, and a reader should be able to see the two numbers
        // rather than take the reading on trust.
        std::size_t deciding_dimension = 0;
        std::size_t deciding_distance = 0;
        std::size_t deciding_bound = 0;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const std::vector<std::size_t> ranks =
                linear_algebra::contraction_ranks(field, tensor.slices, axis);
            const std::size_t bound_here =
                rank_metric_bound::griesmer_bound_on_axis(ranks, characteristic);
            const std::size_t distance_here = rank_metric_bound::minimum_rank_distance(ranks);
            // A strictly better bound wins. Axes that tie on the bound are broken
            // toward the larger `d`, which is the more informative of two axes
            // that agree on the answer; testing `d` before the bound would report
            // an axis the answer did not come from.
            if (bound_here > deciding_bound ||
                (bound_here == deciding_bound && distance_here > deciding_distance)) {
                deciding_bound = bound_here;
                deciding_distance = distance_here;
                deciding_dimension =
                    rank_metric_bound::slice_space_dimension(ranks, characteristic);
            }
            check::equal(label + " axis " + std::to_string(axis) + " slice space dimension",
                         static_cast<long long>(
                             rank_metric_bound::slice_space_dimension(ranks, characteristic)),
                         static_cast<long long>(flattenings[axis]));
            
        }
        // What wiring Griesmer into `rank_lower_bound` would return. A `max` of
        // valid lower bounds is a valid lower bound, so this can only rise; the
        // point of pinning it is that the one fixture where it rises is a checked
        // number and not a sentence in a README.
        const long long floor_today =
            static_cast<long long>(linear_algebra::rank_lower_bound(field, tensor.slices));
        check::equal(label + " floor once Griesmer is wired in",
                     std::max(floor_today, griesmer), fixture.combined_floor);

        std::cout << "  timing  " << label << " in " << std::fixed << std::setprecision(3)
                  << seconds * 1000.0 << " ms, k=" << deciding_dimension
                  << " d=" << deciding_distance << ", against rank_lower_bound " << floor_today
                  << "\n";
    }
    // A slice space smaller than its axis: both bounds read the space, not the
    // axis, so a tensor that is not concise is bounded at its true rank of 1.
    {
        const linear_algebra::Tensor tensor = repeated_slice_tensor();
        const ModularField field(tensor.characteristic);
        check::equal("a repeated slice bounds at one",
                     static_cast<long long>(
                         rank_metric_bound::griesmer_lower_bound(field, tensor.slices)),
                     1);
    }
    // No budget means every axis is skipped, which must weaken the answer to
    // zero rather than enumerate anything or report a bound it did not compute.
    {
        const linear_algebra::Tensor tensor =
            linear_algebra::read_tensor_file(directory + "/matmul_2x2x2.tensor");
        const ModularField field(tensor.characteristic);
        check::equal("no table budget skips every axis",
                     static_cast<long long>(
                         rank_metric_bound::griesmer_lower_bound(field, tensor.slices, 0)),
                     0);
    }
    // Soundness on shapes no fixture reaches. A tensor built from `terms` random
    // rank-one terms has rank at most `terms`, so a bound above `terms` is a false
    // refutation, and this is where the two cases the header argues about actually
    // occur: terms that are linearly dependent, so the rank-one matrices spanning
    // the slice space are not independent, and an axis longer than its slice space,
    // so the contraction has a kernel to take a complement of. Neither is reachable
    // from the fixture list, and both are where a bound derived from a theorem
    // stated for concise tensors would break if the derivation were wrong.
    //
    // One `check` line for the whole sweep unless something fails, since 120
    // passing trials say one thing and should cost one line to say it.
    {
        std::mt19937 generator(20260819);
        std::size_t trials = 0;
        std::size_t violations = 0;
        std::size_t tight = 0;
        for (int64_t characteristic : {2, 3, 5}) {
            const ModularField field(characteristic);
            for (int trial = 0; trial < 40; ++trial) {
                const std::size_t rows = 1 + generator() % 4;
                const std::size_t columns = 1 + generator() % 4;
                const std::size_t depth = 1 + generator() % 4;
                const std::size_t terms = 1 + generator() % 8;
                const std::vector<ModularMatrix> slices =
                    low_rank_tensor(field, characteristic, generator, rows, columns, depth, terms);
                const long long highest =
                    static_cast<long long>(rank_metric_bound::griesmer_lower_bound(field, slices));
                ++trials;
                if (highest == static_cast<long long>(terms)) ++tight;
                if (highest > static_cast<long long>(terms)) {
                    ++violations;
                    check::equal("GF(" + std::to_string(characteristic) + ") trial " +
                                     std::to_string(trial) + " bound above a rank of " +
                                     std::to_string(terms),
                                 highest, static_cast<long long>(terms));
                }
            }
        }
        check::equal("random low-rank tensors tried", static_cast<long long>(trials), 120);
        check::equal("neither bound ever exceeded a random tensor's rank",
                     static_cast<long long>(violations), 0);
        // The sweep has to sit on the boundary to mean anything: a bound that
        // came out far below every rank would pass the line above while proving
        // nothing. One trial in six is bounded at exactly its rank, and the seed
        // is fixed, so this number moving means the shapes moved and the sweep
        // needs looking at rather than re-pinning.
        check::equal("random tensors bounded at exactly their rank",
                     static_cast<long long>(tight), 20);
    }

    return check::report("rank metric bound");
}
