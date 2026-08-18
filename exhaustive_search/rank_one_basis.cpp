#include "rank_one_basis.h"

#include "exhaustive_search.h"
#include "measures.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// Every element of the subspace, each tested for rank one.
std::vector<Matrix> by_walking_the_subspace(const Field& field, const ReducedBasis& span,
                                            std::size_t rows, std::size_t columns,
                                            std::size_t needed, std::size_t elements) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::vector<std::vector<Element>>& basis = span.rows();
    const std::size_t width = rows * columns;

    std::vector<Matrix> found;
    ReducedBasis independent(field, width);
    std::vector<Element> combination(width);

    for (std::size_t index = 1; index < elements; ++index) {
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

std::vector<Matrix> rank_one_basis_of(const Field& field, const ReducedBasis& span,
                                      const std::vector<Matrix>& pool, std::size_t needed,
                                      std::vector<Element>& scratch) {
    if (pool.empty()) return {};
    const std::size_t rows = pool.front().rows();
    const std::size_t columns = pool.front().columns();

    const std::size_t elements = elements_of(field, span.dimension(), pool.size());
    if (elements != 0 && elements < pool.size()) {
        return by_walking_the_subspace(field, span, rows, columns, needed, elements);
    }
    return independent_rank_one_maps_in(field, span, rows * columns, pool, needed, scratch);
}

}  // namespace bilinear_rank
