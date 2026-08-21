/// The price model against the sweeps it was corrected on.
///
/// **No timing is taken here.** The seconds below were measured once, by hand,
/// under [`../../MEASURING.md`](../../MEASURING.md), and are constants in this
/// file; what runs is the model's arithmetic against them. That is deliberate:
/// `MEASURING.md` says no test may fail on a clock, and a model held to recorded
/// observations is exactly as reproducible as the observations.
///
/// What it catches is a change to `canonical_route_price.cpp` that quietly stops
/// agreeing with the ten rows, which is the only way this predicate can rot: it
/// has no caller yet, so nothing else would notice.
#include <cmath>
#include <cstddef>
#include <string>

#include "canonical_route_price.h"
#include "check.h"

namespace {

/// One shape, one level, both routes. `plain` and `canonical` are seconds, fastest
/// of three, five for the last row; the node and canonisation counts beside them are
/// exact and machine
/// independent, and are what a reader should check a rebuild against first.
struct Observation {
    const char* name;
    bilinear_rank::RouteShape shape;
    std::size_t plain_nodes;
    std::size_t canonical_nodes;
    std::size_t canonisations;
    double plain;
    double canonical;
};

/// Measured 2026-08-21 on the reference machine, `factor-over-canonical-basis
/// --route exhaustive|canonical --floor t --ceiling t`, the lock held. The machine
/// was **not** idle — `/proc/loadavg` ran 1.2 to 2.6 — so the absolute seconds are
/// over-estimates. Both routes of a row ran back to back against the same
/// background, so the ratio within a row is what these are evidence of.
const Observation observations[] = {
    {"<2,2,2> at 5", {2, 2, 2, 2, 5, 6}, 6, 6, 10, 0.000203193, 0.000557479},
    {"<2,2,2> at 6", {2, 2, 2, 2, 6, 6}, 648, 58, 784, 0.00142795, 0.0164453},
    {"<2,2,2> at 7", {2, 2, 2, 2, 7, 6}, 3167, 14, 2146, 0.00594943, 0.0427039},
    {"<2,2,3> at 7", {2, 2, 2, 3, 7, 6}, 6, 6, 10, 0.000909333, 0.0025114},
    {"<2,2,3> at 8", {2, 2, 2, 3, 8, 6}, 2748, 85, 1092, 0.00789853, 0.0926958},
    {"<2,2,4> at 9", {2, 2, 2, 4, 9, 6}, 6, 6, 10, 0.00715115, 0.0128771},
    {"<2,2,4> at 10", {2, 2, 2, 4, 10, 6}, 11130, 92, 1125, 0.0778568, 0.500045},
    {"<2,3,3> at 7", {2, 2, 3, 3, 7, 6}, 11, 11, 20, 0.105456, 0.197995},
    {"<2,3,3> at 8", {2, 2, 3, 3, 8, 6}, 229870, 1083, 25664, 0.847374, 98.0218},
    {"<3,3,3> at 10", {2, 3, 3, 3, 10, 6}, 14, 14, 26, 4.91855, 3.69509},
};

/// Whether two numbers are within a factor, which is the only claim a model with a
/// measured setwise stabiliser in it is entitled to make.
void within(const std::string& what, double actual, double expected, double factor) {
    const double ratio = actual > expected ? actual / expected : expected / actual;
    if (ratio <= factor) {
        std::cout << "  ok    " << what << " = " << actual << ", measured " << expected << " ("
                  << ratio << "x)\n";
    } else {
        std::cout << "  FAIL  " << what << " = " << actual << ", measured " << expected << " ("
                  << ratio << "x, over " << factor << "x)\n";
        ++check::failure_count;
    }
}

}  // namespace

int main() {
    const bilinear_rank::CanonicalPrices prices;

    // Nine of the ten verdicts, and the tenth named rather than fitted. `<3,3,3>`
    // at 10 is the one row where canonical augmentation wins, and it wins with a
    // saving of exactly nothing: both routes visit 14 nodes. The win is one
    // `orbit_representatives` call replacing 261 121 per-element generator tests,
    // which is a mechanism this model does not carry, and bending a constant until
    // one row flipped would have hidden that rather than found it.
    std::cout << "the verdict, against which route actually finished first\n";
    std::size_t misses = 0;
    for (const Observation& seen : observations) {
        const bilinear_rank::RouteVerdict verdict =
            bilinear_rank::price_canonical_route(seen.shape, prices);
        const bool measured = seen.canonical < seen.plain;
        if (verdict.pays == measured) {
            std::cout << "  ok    " << seen.name << (measured ? " pays" : " does not pay") << "\n";
        } else {
            std::cout << "  miss  " << seen.name << ": predicted "
                      << (verdict.pays ? "pays" : "does not pay") << ", measured "
                      << seen.canonical / seen.plain << "x\n";
            ++misses;
        }
    }
    check::equal("rows the predicate gets wrong", static_cast<long long>(misses), 1);

    std::cout << "the predicted cost, against the ratio the two routes measured\n";
    for (const Observation& seen : observations) {
        const bilinear_rank::RouteVerdict verdict =
            bilinear_rank::price_canonical_route(seen.shape, prices);
        within(std::string(seen.name) + " canonical over plain", verdict.predicted_cost,
               seen.canonical / seen.plain, 20);
    }

    // The correction this model exists to carry. Orbit counting bounds the saving
    // by `|G|`, and taking the bound as the estimate is what the first version did:
    // at one level of augmentation the baseline already emits one node per pool
    // orbit and `rho` is exactly 1, not 216 or 6 048. Re-imposing the bound has to
    // break rows, or the correction was not one.
    std::cout << "the uncorrected saving side, which took the orbit-counting cap\n";
    std::size_t disagreements = 0;
    for (const Observation& seen : observations) {
        const bilinear_rank::RouteVerdict verdict =
            bilinear_rank::price_canonical_route(seen.shape, prices);
        const double uncorrected = verdict.price_ratio / verdict.group_order;
        const bool measured_pays = seen.canonical < seen.plain;
        if ((uncorrected < 1) != measured_pays) ++disagreements;
    }
    check::equal("rows the |G| cap gets wrong", static_cast<long long>(disagreements), 7);

    // `rho` is 1 at one level of augmentation, whatever the group, because the
    // baseline's single-generator rejection already produced one node per orbit.
    // Every one-level row above measured exactly equal node counts, which is the
    // fact the model has to reproduce rather than approximate.
    std::cout << "one level of augmentation removes nothing\n";
    for (const Observation& seen : observations) {
        if (seen.shape.target != seen.shape.rows * seen.shape.columns + 1) continue;
        check::equal(std::string(seen.name) + " plain nodes equal canonical nodes",
                     static_cast<long long>(seen.plain_nodes),
                     static_cast<long long>(seen.canonical_nodes));
        const bilinear_rank::RouteVerdict verdict =
            bilinear_rank::price_canonical_route(seen.shape, prices);
        check::equal(std::string(seen.name) + " modelled saving",
                     static_cast<long long>(std::llround(verdict.saving_ratio)), 1);
    }

    return check::report("route_price");
}
