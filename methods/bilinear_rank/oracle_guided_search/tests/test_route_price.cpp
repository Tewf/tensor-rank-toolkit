/// The price model against the sweeps it was corrected on.
///
/// **No timing is taken here.** The seconds below were measured once, by hand,
/// under [`../../MEASURING.md`](../../MEASURING.md), and are constants in this
/// file; what runs is the model's arithmetic against them. That is deliberate:
/// `MEASURING.md` says no test may fail on a clock, and a model held to recorded
/// observations is exactly as reproducible as the observations.
///
/// The counts beside them are not observations of this machine at all. Node
/// counts, canonisation counts and orbit sizes are facts about the problem, they
/// come out the same anywhere, and they are what a reader should check a rebuild
/// against first.
///
/// What it catches is a change to `canonical_route_price.cpp` that quietly stops
/// agreeing with the ten rows, and (since 2026-08-21) a change that makes the
/// one-level clause fire for a reason other than the one it was derived from.
#include <cmath>
#include <cstddef>
#include <string>

#include "canonical_route_price.h"
#include "check.h"

namespace {

/// One shape, one level, both routes. `plain` and `canonical` are seconds, fastest
/// of three; the node, canonisation and orbit counts beside them are exact and
/// machine independent.
struct Observation {
    const char* name;
    bilinear_rank::RouteShape shape;
    std::size_t plain_nodes;
    std::size_t canonical_nodes;
    std::size_t canonisations;
    /// The orbits of the pool under the stabiliser. Zero above one level of
    /// augmentation, where nothing reads them.
    bilinear_rank::PoolOrbits orbits;
    double plain;
    double canonical;
};

/// Measured 2026-08-21 on the reference machine, `factor-over-canonical-basis
/// --route exhaustive|canonical --floor t --ceiling t`, the lock held, fastest of
/// three, both routes of a row back to back. The machine was **not** idle
/// (`/proc/loadavg` ran 1.8 to 2.1), so the absolute seconds are over-estimates
/// and a ratio within a row is what they are evidence of. Every node and
/// canonisation count reproduces the run of the day before exactly.
///
/// The orbit counts come from one `orbit_representatives` pass over each pool
/// under the stabiliser `factor-over-canonical-basis` builds.
const Observation observations[] = {
    {"<2,2,2> at 5", {2, 2, 2, 2, 5, 6}, 6, 6, 6, {5, 10773.0}, 0.000216731, 0.000597702},
    {"<2,2,2> at 6", {2, 2, 2, 2, 6, 6}, 648, 58, 483, {}, 0.00141061, 0.00920869},
    {"<2,2,2> at 7", {2, 2, 2, 2, 7, 6}, 3167, 14, 1451, {}, 0.00581706, 0.0267977},
    {"<2,2,3> at 7", {2, 2, 2, 3, 7, 6}, 6, 6, 6, {5, 242109.0}, 0.000649117, 0.00178146},
    {"<2,2,3> at 8", {2, 2, 2, 3, 8, 6}, 2748, 85, 676, {}, 0.00796475, 0.0347442},
    {"<2,2,4> at 9", {2, 2, 2, 4, 9, 6}, 6, 6, 6, {5, 5323725.0}, 0.00519425, 0.00889207},
    {"<2,2,4> at 10", {2, 2, 2, 4, 10, 6}, 11130, 92, 698, {}, 0.0702197, 0.136451},
    {"<2,3,3> at 7", {2, 2, 3, 3, 7, 6}, 11, 11, 11, {10, 172547865.0}, 0.0971953, 0.120164},
    {"<2,3,3> at 8", {2, 2, 3, 3, 8, 6}, 229870, 1083, 15754, {}, 0.768972, 11.6248},
    {"<3,3,3> at 10", {2, 3, 3, 3, 10, 6}, 14, 14, 14, {13, 9939450913.0}, 4.87481, 2.26021},
};

/// How far a prediction is from the row it was taken against, which is the only
/// claim a model with a measured setwise stabiliser in it is entitled to make.
double apart(double actual, double expected) {
    return actual > expected ? actual / expected : expected / actual;
}

bool one_level(const Observation& seen) {
    return seen.shape.target == seen.shape.rows * seen.shape.columns + 1;
}

/// The verdict for one row, with the prices and orbit statistics a caller chooses.
bool pays(const Observation& seen, const bilinear_rank::CanonicalPrices& prices,
          const bilinear_rank::PoolOrbits& orbits) {
    return bilinear_rank::price_canonical_route(seen.shape, prices, orbits).pays;
}

}  // namespace

int main() {
    const bilinear_rank::CanonicalPrices prices;

    // **Every one of the ten, since the one-level clause stopped pricing a root as
    // though it were a tree.** It used to be nine, with `<3,3,3>` at 10 named
    // rather than modelled: canonical augmentation wins there by 2.15x with a node
    // saving of exactly nothing, and the margin is in how each route names its
    // root's children. `least_in_orbit` costs `O(sum |O_i|^2)` and
    // `orbit_representatives` costs `O(|P|)` for the identical answer, which is a
    // difference in order and not a constant anybody may fit away.
    std::cout << "the verdict, against which route actually finished first\n";
    std::size_t misses = 0;
    for (const Observation& seen : observations) {
        const bool measured = seen.canonical < seen.plain;
        if (pays(seen, prices, seen.orbits) == measured) {
            std::cout << "  ok    " << seen.name << (measured ? " pays" : " does not pay") << "\n";
        } else {
            std::cout << "  miss  " << seen.name << ": measured " << seen.canonical / seen.plain
                      << "x\n";
            ++misses;
        }
    }
    check::equal("rows the predicate gets wrong", static_cast<long long>(misses), 0);

    // Nine rows inside a factor of twelve and **one named rather than widened
    // to**. `<2,3,3>` at 8 is the tree regime's worst row and always was: the
    // fitted `rho = (scans a node)^(levels-1)` reads 966 where the sweep measures
    // 212, and the per-node price reads 1 100 where the sweep measures 3 209, so
    // the two errors compound instead of cancelling. Moving the tolerance until it
    // fits would have hidden the defect the way taking `rho <= |G|` as an estimate
    // once did.
    std::cout << "the predicted cost, against the ratio the two routes measured\n";
    std::size_t outside = 0;
    std::string worst;
    for (const Observation& seen : observations) {
        const bilinear_rank::RouteVerdict verdict =
            bilinear_rank::price_canonical_route(seen.shape, prices, seen.orbits);
        const double error = apart(verdict.predicted_cost, seen.canonical / seen.plain);
        std::cout << (error <= 12 ? "  ok    " : "  wide  ") << seen.name << " = "
                  << verdict.predicted_cost << ", measured " << seen.canonical / seen.plain << " ("
                  << error << "x)\n";
        if (error > 12) {
            ++outside;
            worst = seen.name;
        }
    }
    check::equal("rows the model is more than 12x out on", static_cast<long long>(outside), 1);
    check::text("and the row it is out on", worst, "<2,3,3> at 8");

    // The correction this model exists to carry. Orbit counting bounds the saving
    // by `|G|`, and taking the bound as the estimate is what the first version did:
    // at one level of augmentation the baseline already emits one node per pool
    // orbit and `rho` is exactly 1, not 216 or 6 048. Re-imposing the bound has to
    // break rows, or the correction was not one.
    std::cout << "the uncorrected saving side, which took the orbit-counting cap\n";
    std::size_t disagreements = 0;
    for (const Observation& seen : observations) {
        const bilinear_rank::RouteVerdict verdict =
            bilinear_rank::price_canonical_route(seen.shape, prices, seen.orbits);
        const double uncorrected = verdict.price_ratio / verdict.group_order;
        if ((uncorrected < 1) != (seen.canonical < seen.plain)) ++disagreements;
    }
    check::equal("rows the |G| cap gets wrong", static_cast<long long>(disagreements), 9);

    // `rho` is 1 at one level of augmentation, whatever the group, because both
    // routes emit exactly one node per `G`-orbit of the pool. That is not an
    // approximation and it is checked as an identity: the orbits were counted by a
    // separate pass, and **orbits + 1 is the node count of both routes**, at all
    // five shapes, root included.
    std::cout << "one level of augmentation: one node per orbit, and the same node twice\n";
    for (const Observation& seen : observations) {
        if (!one_level(seen)) continue;
        check::equal(std::string(seen.name) + " plain nodes equal canonical nodes",
                     static_cast<long long>(seen.plain_nodes),
                     static_cast<long long>(seen.canonical_nodes));
        check::equal(std::string(seen.name) + " nodes are the orbits and the root",
                     static_cast<long long>(seen.plain_nodes),
                     static_cast<long long>(seen.orbits.count + 1));
        const bilinear_rank::RouteVerdict verdict =
            bilinear_rank::price_canonical_route(seen.shape, prices, seen.orbits);
        check::equal(std::string(seen.name) + " modelled saving",
                     static_cast<long long>(std::llround(verdict.saving_ratio)), 1);
    }

    // **The decision itself, pinned.** These three are what a caller wired to this
    // predicate will do, and they are stated as the shapes rather than derived from
    // the loop above so that a rewrite of the loop cannot quietly take them with it.
    std::cout << "the decision, on three shapes by name\n";
    const bilinear_rank::RouteShape small{2, 2, 2, 2, 5, 6};
    const bilinear_rank::RouteShape middle{2, 2, 3, 3, 7, 6};
    const bilinear_rank::RouteShape large{2, 3, 3, 3, 10, 6};
    check::equal("<2,2,2> at 5 takes the plain route",
                 bilinear_rank::price_canonical_route(small, prices, {5, 10773.0}).pays, 0);
    check::equal("<2,3,3> at 7 takes the plain route",
                 bilinear_rank::price_canonical_route(middle, prices, {10, 172547865.0}).pays, 0);
    check::equal("<3,3,3> at 10 takes the canonical route",
                 bilinear_rank::price_canonical_route(large, prices, {13, 9939450913.0}).pays, 1);

    // **Two sabotages, because a decision nobody can break is a decision nobody is
    // reading.** The first withholds the orbit statistics, which are the one input
    // to this clause that costs anything to obtain; the second says the baseline's
    // orbit test is linear rather than quadratic, which is what removing the
    // `std::find` in `least_in_orbit` would make true. Either must take the one
    // shape that pays off the canonical route, or the clause is resting on
    // something other than what it claims.
    std::cout << "sabotage: the clause has to stop firing when its reason is removed\n";
    check::equal("<3,3,3> with the orbits unmeasured",
                 bilinear_rank::price_canonical_route(large, prices, {}).pays, 0);
    bilinear_rank::CanonicalPrices cheap_orbit_test = prices;
    cheap_orbit_test.orbit_test_picoseconds = prices.orbit_test_picoseconds / 100;
    check::equal(
        "<3,3,3> with the baseline's orbit test 100x cheaper",
        bilinear_rank::price_canonical_route(large, cheap_orbit_test, {13, 9939450913.0}).pays, 0);

    return check::report("route_price");
}
