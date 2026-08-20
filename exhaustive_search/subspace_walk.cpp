#include "subspace_walk.h"

#include "measures.h"
#include "reflected_gray_walk.h"
#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// The rank-one test and the independence test, which the two walks share and
/// which is the only place either of them can say yes.
///
/// Written once so that the routes cannot come to disagree about what a
/// rank-one map is while agreeing about which elements they visit.
///
/// **Both tests read the combination buffer the walk already carries, and the
/// `Matrix` is built only for an element that is kept.** Almost none are: the
/// answer for the overwhelming majority is no at the second row, and forming a
/// matrix to be told so cost an allocation per element, a `SpanBasis` per
/// element, a copy per row of it and an elimination that ran to the end of a
/// question already settled. `is_rank_one` allocates nothing and stops at the
/// first entry that disagrees; the verdicts are the same ones, since it is the
/// same predicate as [`rank`](../linear_algebra/measures.h) `== 1` and is held
/// against it over every small matrix in
/// [`tests/test_rank_one_predicate.cpp`](tests/test_rank_one_predicate.cpp).
bool keep_if_rank_one(const Field& field, const std::vector<Element>& combination,
                      std::size_t rows, std::size_t columns, ReducedBasis& independent,
                      std::vector<Matrix>& found) {
    if (!linear_algebra::is_rank_one(field, combination.data(), rows, columns)) return false;
    // The flattened element is the combination itself, which is what the span is
    // built over, so the independence test needs no matrix either.
    if (!independent.try_add(combination)) return false;

    Matrix element(rows, columns);
    for (std::size_t entry = 0; entry < combination.size(); ++entry) {
        element.data()[entry] = combination[entry];
    }
    found.push_back(std::move(element));
    return true;
}

}  // namespace

std::vector<Matrix> by_walking_the_subspace(const Field& field, const ReducedBasis& span,
                                            std::size_t rows, std::size_t columns,
                                            std::size_t needed, std::size_t elements,
                                            SearchBudget* budget) {
    const auto radix = static_cast<std::size_t>(field.characteristic());
    const std::vector<std::vector<Element>>& basis = span.rows();
    const std::size_t width = rows * columns;

    std::vector<Matrix> found;
    ReducedBasis independent(field, width);

    // The combination is the subspace element the walk currently stands on, and
    // it is carried from one element to the next rather than rebuilt. It starts
    // where the walk does, on the all-zero string, which is the zero map.
    std::vector<Element> combination(width);
    for (Element& entry : combination) field.assign(entry, field.zero);

    ReflectedGrayWalk walk(basis.size(), radix);
    ReflectedGrayWalk::Step step;

    for (std::size_t index = 1; index < elements; ++index) {
        if (budget != nullptr && !budget->may_examine(index)) break;
        // `elements` is `radix ^ basis.size()` and the walk visits exactly that
        // many strings, so this never stops the loop first. It is asked anyway
        // rather than assumed, since the two counts are computed apart.
        if (!walk.advance(step)) break;

        // One digit moved by one, so one row goes in or comes out. No multiply.
        const std::vector<Element>& row = basis[step.digit];
        if (step.upward) {
            for (std::size_t column = 0; column < width; ++column) {
                field.addin(combination[column], row[column]);
            }
        } else {
            for (std::size_t column = 0; column < width; ++column) {
                field.subin(combination[column], row[column]);
            }
        }

        if (keep_if_rank_one(field, combination, rows, columns, independent, found)) {
            if (found.size() == needed) break;
        }
    }
    return found;
}

std::vector<Matrix> by_rebuilding_each_element(const Field& field, const ReducedBasis& span,
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

        if (keep_if_rank_one(field, combination, rows, columns, independent, found)) {
            if (found.size() == needed) break;
        }
    }
    return found;
}

}  // namespace bilinear_rank
