#include <limits>

#include "rank_one_basis.h"

#include "candidate_pool.h"
#include "exhaustive_search.h"
#include "subspace_walk.h"

namespace bilinear_rank {

namespace {

/// How many elements the subspace has, or zero if that overflows what is worth
/// counting.
std::size_t elements_of(const Field& field, std::size_t dimension, std::size_t ceiling) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    std::size_t count = 1;
    for (std::size_t step = 0; step < dimension; ++step) {
        if (count > ceiling / characteristic) return 0;
        count *= characteristic;
    }
    return count;
}

LeafRoute chosen_route = LeafRoute::Auto;

}  // namespace

void set_leaf_route(LeafRoute route) { chosen_route = route; }
LeafRoute leaf_route() { return chosen_route; }

template <typename Candidates>
std::vector<Matrix> rank_one_basis_of(const Field& field, const ReducedBasis& span,
                                      const Candidates& pool, std::size_t needed,
                                      std::vector<Element>& scratch, SearchBudget* budget,
                                      const Gf2Leaf<Candidates>* binary) {
    if (pool.size() == 0) return {};
    const Matrix& first = pool[0];
    const std::size_t rows = first.rows();
    const std::size_t columns = first.columns();

    // One rule, asked before the field is: which route is cheaper here is a fact
    // about the shape and the dimension, and it is the same fact over GF(2).
    // `elements_of` stops counting once `p^dim` passes its ceiling, so passing
    // the pool size makes the count and the comparison the same act: a non-zero
    // answer already means the walk is the smaller side. A forced walk has to be
    // counted against a real ceiling instead, or it could never be forced, since
    // the only leaves worth forcing are the ones the rule sends to the pool.
    const std::size_t ceiling =
        chosen_route == LeafRoute::Walk ? std::numeric_limits<std::size_t>::max() : pool.size();
    const std::size_t elements = elements_of(field, span.dimension(), ceiling);
    const bool walk = elements != 0
                      && (chosen_route == LeafRoute::Walk || (chosen_route == LeafRoute::Auto
                                                              && elements < pool.size()));
    if (walk) {
        if (binary != nullptr) return binary->by_walking_the_subspace(span, needed, elements, budget);
        return by_walking_the_subspace(field, span, rows, columns, needed, elements, budget);
    }
    if (binary != nullptr) return binary->by_scanning_the_pool(span, needed, budget);
    return independent_rank_one_maps_in(field, span, rows * columns, pool, needed, scratch, budget);
}

template std::vector<Matrix> rank_one_basis_of(const Field&, const ReducedBasis&,
                                              const std::vector<Matrix>&, std::size_t,
                                              std::vector<Element>&, SearchBudget*,
                                              const Gf2Leaf<std::vector<Matrix>>*);
template std::vector<Matrix> rank_one_basis_of(const Field&, const ReducedBasis&,
                                              const Addressed&, std::size_t,
                                              std::vector<Element>&, SearchBudget*,
                                              const Gf2Leaf<Addressed>*);

}  // namespace bilinear_rank
