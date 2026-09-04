#include "fewest_products.h"

#include <stdexcept>

#include "measures.h"
#include "rank_lower_bound.h"

namespace bilinear_rank {

std::size_t flattening_floor(const Field& field, const std::vector<Matrix>& base) {
    if (base.empty()) return 0;
    // The name predates `[yang2025]`'s rank sums: this returns the fuller
    // `rank_lower_bound` below, not a flattening rank alone.
    //
    // The third flattening is the slice space, so its rank is exactly the span
    // dimension this used to start from; the maximum over the three axes can only
    // be larger. Every target below it is refuted by Gaussian elimination, so
    // asking the tree search about one is spending exponential time on a
    // polynomial question.
    //
    // Since `[yang2025]`'s two rank sums joined it, this is the maximum over three
    // bounds rather than one. On every fixture here one of the rank sums wins or
    // ties, by as much as 4 to 8 on GF(16) and 10 to 14 on `f2_3x8`; the
    // flattening stays because it is the only polynomial one of the three.
    return linear_algebra::rank_lower_bound(field, base);
}

std::string require_bound_consistent(std::size_t products_found, std::size_t bound) {
    const std::string counted =
        std::to_string(products_found) + " products, rank bound " + std::to_string(bound);
    if (products_found < bound) {
        throw std::logic_error(counted + ", so one of the two is wrong");
    }
    return counted + ", gap " + std::to_string(products_found - bound);
}

bool fewest_products_by_sweep(const Field& field, const std::vector<Matrix>& base,
                              const std::vector<Matrix>& pool, SearchBudget& budget,
                              std::vector<Matrix>& products) {
    if (base.empty()) return false;
    const std::size_t lowest = flattening_floor(field, base);
    const std::size_t highest = linear_algebra::multiplication_count(field, base);

    for (std::size_t target = lowest; target <= highest; ++target) {
        if (expand_subspace(field, base, pool, 0, target, budget, products)) return true;
        if (!budget.tree_fully_walked) return false;
    }
    return false;
}

}  // namespace bilinear_rank
