#include "rank_one_basis.h"

#include "candidate_pool.h"
#include "exhaustive_search.h"
#include "measures.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// Every element of the subspace, each tested for rank one.
std::vector<Matrix> by_walking_the_subspace(const Field& field, const ReducedBasis& span,
                                            std::size_t rows, std::size_t columns,
                                            std::size_t needed, std::size_t elements,
                                            SearchBudget* budget) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::vector<std::vector<Element>>& basis = span.rows();
    const std::size_t width = rows * columns;

    std::vector<Matrix> found;
    ReducedBasis independent(field, width);
    std::vector<Element> combination(width);

    for (std::size_t index = 1; index < elements; ++index) {
        if (budget != nullptr && !budget->may_examine(index)) break;
        for (Element& entry : combination) field.assign(entry, field.zero);

        std::size_t remaining = index;
        for (const std::vector<Element>& row : basis) {
            const auto digit = static_cast<Element>(remaining % characteristic);
            remaining /= characteristic;
            if (field.isZero(digit)) continue;
            for (std::size_t column = 0; column < width; ++column) {
                field.axpyin(combination[column], digit, row[column]);
            }
        }

        Matrix element(rows, columns);
        for (std::size_t entry = 0; entry < width; ++entry) element.data()[entry] = combination[entry];
        if (linear_algebra::rank(field, element) != 1) continue;

        if (independent.try_add(element)) {
            found.push_back(std::move(element));
            if (found.size() == needed) break;
        }
    }
    return found;
}

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

}  // namespace

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
    const std::size_t elements = elements_of(field, span.dimension(), pool.size());
    if (elements != 0 && elements < pool.size()) {
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
