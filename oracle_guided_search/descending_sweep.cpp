#include "descending_sweep.h"

#include "timing.h"

namespace bilinear_rank {

std::size_t naive_ceiling(const linear_algebra::Tensor& tensor) {
    // One multiplication per entry of the flattened bilinear form: the rank of the
    // map is at most the number of rank-one maps it takes to write every slice out
    // coordinate by coordinate, which is rows times columns.
    return tensor.rows() * tensor.columns();
}

SweepResult descend_from_ceiling(const linear_algebra::Tensor& tensor, std::size_t ceiling,
                                 const FinderSettings& settings) {
    const cli::Clock::time_point started = cli::Clock::now();

    SweepResult sweep;
    sweep.floor = settings.floor;

    for (std::size_t products = ceiling; products >= 1; --products) {
        // Below the floor nothing exists, so the budget would be spent proving
        // what a polynomial-time bound already said.
        if (settings.floor > products) break;

        FoundAtRank step = find_at_rank(tensor, products, settings);
        const bool found = was_found(step.outcome);
        if (found) {
            sweep.upper = products;
            sweep.decomposition = step.decomposition;
            sweep.algorithm = step.algorithm;
        }
        sweep.steps.push_back(std::move(step));
        if (!found) break;

        // What came back may be shorter than what was asked for: the pre-test
        // returns the same object whatever `k` is, and a solver asked for more
        // terms than the map needs spends the difference on terms that are zero.
        // Either way, asking again just below `k` would rediscover it, so the
        // descent resumes below what was actually delivered.
        const FoundAtRank& taken = sweep.steps.back();
        // A zero map needs no products at all, and there is nothing below zero to
        // walk down to. Stopping here also keeps the decrement below from wrapping.
        if (taken.decomposition.empty()) {
            sweep.upper = 0;
            break;
        }
        if (taken.decomposition.size() < products) {
            sweep.upper = taken.decomposition.size();
            products = taken.decomposition.size();
        }
    }

    sweep.seconds = cli::elapsed_seconds(started);
    return sweep;
}

}  // namespace bilinear_rank
