#include "canonical_route_price.h"

#include <algorithm>
#include <cmath>

namespace bilinear_rank {

namespace {

/// `(p^length - 1)/(p - 1)`: one normalised vector per scalar class, which is one
/// axis of the pool grid and half of the factored presentation's domain.
double normalised_count(std::size_t characteristic, std::size_t length) {
    return (std::pow(static_cast<double>(characteristic), static_cast<double>(length)) - 1) /
           static_cast<double>(characteristic - 1);
}

/// `C(pool, taken)`, in doubles because the counts are astronomical by design:
/// `C(261121, 5)` is 1.0e26, and it has to be compared against a presentation fee
/// of milliseconds.
double subsets_of(double pool, std::size_t taken) {
    double count = 1;
    for (std::size_t step = 0; step < taken; ++step) {
        count *= (pool - static_cast<double>(step)) / static_cast<double>(step + 1);
    }
    return count;
}

}  // namespace

double general_linear_order(std::size_t characteristic, std::size_t order) {
    const double prime = static_cast<double>(characteristic);
    const double full = std::pow(prime, static_cast<double>(order));
    double total = 1;
    for (std::size_t taken = 0; taken < order; ++taken) {
        total *= full - std::pow(prime, static_cast<double>(taken));
    }
    return total;
}

double product_group_order(const RouteShape& shape) {
    const double scalars = static_cast<double>(shape.characteristic - 1);
    return general_linear_order(shape.characteristic, shape.rows) *
           general_linear_order(shape.characteristic, shape.inner) *
           general_linear_order(shape.characteristic, shape.columns) / (scalars * scalars);
}

/// The break-even, assembled from the shape alone.
///
/// Each block is one clause of the derivation in
/// [`when-canonical-pays/`](when-canonical-pays/README.md), in the order that file
/// argues them: what one membership test costs, what a plain node costs, what a
/// canonical node costs, what the presentation costs, and only then the two ratios.
RouteVerdict price_canonical_route(const RouteShape& shape, const CanonicalPrices& prices) {
    RouteVerdict verdict;
    const double lefts = normalised_count(shape.characteristic, shape.rows * shape.inner);
    const double rights = normalised_count(shape.characteristic, shape.inner * shape.columns);
    verdict.pool_size = static_cast<std::size_t>(lefts * rights);
    verdict.degree = static_cast<std::size_t>(lefts + rights);
    verdict.group_order = product_group_order(shape);

    const std::size_t span = shape.rows * shape.columns;
    if (shape.target <= span) {
        verdict.refusal = "the target is at or below the tensor's own span, so there is no level "
                          "to augment and no duplication to remove";
        return verdict;
    }
    verdict.levels = shape.target - span;

    // A membership test costs one reduction of a matrix against a basis, so it
    // scales with the span dimension and the entries of a slice, and with nothing
    // else. That product is the unit every cost here is quoted in.
    const double work = static_cast<double>(span * shape.rows * shape.inner * shape.inner *
                                            shape.columns);
    const double picosecond = 1e-12;
    verdict.plain_node_seconds =
        static_cast<double>(prices.plain_node_picoseconds) * work * picosecond;
    verdict.pool_scan_seconds = static_cast<double>(prices.membership_picoseconds) * work *
                                lefts * rights * picosecond;

    const double nanosecond = 1e-9;
    const double degree = static_cast<double>(verdict.degree);
    verdict.image_seconds = (static_cast<double>(prices.image_floor_nanoseconds) +
                             static_cast<double>(prices.image_nanoseconds_per_point) * degree) *
                            nanosecond;
    verdict.stabiliser_seconds =
        static_cast<double>(prices.stabiliser_nanoseconds_per_point) * degree * nanosecond;
    verdict.presentation_seconds =
        static_cast<double>(prices.presentation_nanoseconds_per_point) * degree * nanosecond;

    // Candidate parents per test: the hyperplanes of the quotient, one per nonzero
    // functional up to scalar, plus one canonical image for the distinguished cell.
    // `candidate_parents` keeps only the reachable ones, so this is an upper bound.
    const double parents = normalised_count(shape.characteristic, verdict.levels) + 1;
    const double branching = static_cast<double>(prices.branching);

    // What `descend` spends at one canonical node: **one** pass over the pool, one
    // setwise stabiliser, and one canonical image per candidate parent per
    // candidate child.
    //
    // It was `c + 2` passes until `pool_cosets.h` replaced them. The pass is a
    // `PoolCosets` at an internal node and the leaf scan in
    // `independent_rank_one_maps_in` at a leaf, and every node is one or the other,
    // so one is the right count either way.
    verdict.canonical_node_seconds = verdict.pool_scan_seconds + verdict.stabiliser_seconds +
                                     branching * parents * verdict.image_seconds;

    // What the baseline would spend: the sweep's levels, each about `C(|P|, j)`
    // subsets thinned by the generator-orbit rejection it already applies.
    double plain_nodes = 0;
    for (std::size_t level = 1; level <= verdict.levels; ++level) {
        plain_nodes += subsets_of(lefts * rights, level);
    }
    plain_nodes /= static_cast<double>(shape.generators);
    const double plain_sweep = plain_nodes * verdict.plain_node_seconds;

    verdict.price_ratio = verdict.canonical_node_seconds / verdict.plain_node_seconds;

    // Orbit counting caps the saving: the `G`-orbits on the `j`-subsets of the pool
    // number at least `C(|P|,j)/|G|`, so the plain tree is at most `|G|` times the
    // canonical one.
    //
    // **The cap is nowhere near attained, and taking it as the estimate was wrong
    // by 205x at `<2,2,3>`.** The measured `rho` reaches 5%, 0.5%, 0.017% and 0.13%
    // of `|G|` at the four shapes swept. What it tracks instead is the scans-to-node
    // ratio, once per level after the first: measured 6.8, 28.4, 115 and 966 scans a
    // node, `rho` came out 11.2, 32.3, 121 and 212.
    // [`when-canonical-pays/against-the-sweeps.md`](when-canonical-pays/against-the-sweeps.md)
    // has the table.
    const double scans_a_node = verdict.pool_scan_seconds / verdict.plain_node_seconds;
    verdict.saving_ratio = std::min(
        verdict.group_order, std::pow(scans_a_node, static_cast<double>(verdict.levels) - 1));

    if (verdict.levels == 1) {
        // **One level is not a tree and the per-node ratio does not apply to it.**
        // The sweep is a root and its children, the baseline's single-generator
        // rejection already emits one child per pool orbit, and `rho` is therefore
        // exactly 1 with nothing left to remove. What is left is the root's own
        // work, against a plain root that scans the pool once and hands the same
        // children on: both routes then pay the same for the leaves.
        verdict.predicted_cost =
            (verdict.canonical_node_seconds + branching * verdict.pool_scan_seconds) /
            ((1 + branching) * verdict.pool_scan_seconds);
    } else {
        verdict.predicted_cost = verdict.price_ratio / verdict.saving_ratio;
    }
    verdict.predicted_cost += verdict.presentation_seconds / plain_sweep;

    if (plain_sweep <= verdict.presentation_seconds) {
        verdict.refusal = "presenting the group costs more than the whole plain sweep, so no node "
                          "saving can pay for it";
        return verdict;
    }
    if (verdict.levels == 1 && verdict.predicted_cost >= 1) {
        verdict.refusal = "one level of augmentation: the baseline already emits one child per "
                          "pool orbit, so there is no duplication left for a parent test to remove";
        return verdict;
    }
    if (verdict.predicted_cost >= 1) {
        verdict.refusal = "the nodes the group removes do not cover the parent tests they cost";
        return verdict;
    }
    verdict.pays = true;
    return verdict;
}

}  // namespace bilinear_rank
