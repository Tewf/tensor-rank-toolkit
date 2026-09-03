#pragma once

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "measures.h"
#include "memory_budget.h"

/// The sparsest basis over a **finite** field, exactly, and cheaply where the
/// operator is narrow.
///
/// `nnz(U V)` over invertible `V` is the sum of the weights of the columns of
/// `U V`, and those columns are exactly some basis of `U`'s column space; every
/// basis arises from some invertible `V`. So the problem *is* "choose a basis of
/// this space of least total weight", and over a finite field that space has
/// `q^k` elements and can be walked.
///
/// **Linear independence is a matroid** `[oxley, Prop. 1.1.1]`, **and the greedy
/// returns a minimum-weight basis of any matroid under any weight function**
/// `[oxley, Lem. 1.8.3]`. So walking the space in ascending weight and keeping
/// whatever is not already spanned is not a heuristic: it is the optimum, and it
/// does not depend on how ties are broken.
///
/// **This is the same algorithm as
/// [`../bilinear_rank/greedy_heuristic/minimum_weight_basis.h`](../bilinear_rank/greedy_heuristic/minimum_weight_basis.h)
/// under a different weight.** There the ground set is `span(T)` and the weight
/// of an element is its *rank*; here it is the operator's column space and the
/// weight is its *number of nonzeros*. One matroid, two costs, and the same
/// theorem. That the two are not one function yet is a duplication this file
/// names rather than hides: unifying them means lifting that header out of
/// `methods/bilinear_rank/greedy_heuristic/` into `core/linear_algebra/` and giving it the weight as an
/// argument, which is its own change with its own measurement.
///
/// **Why the rational routes do not do this.** They work over `Q`, where
/// the column space is infinite and cannot be walked, which is what forces
/// `[beniamini2020]`'s oracles to search column subsets and to assemble greedily
/// one row at a time without an optimality guarantee. The finite field is what
/// makes the exact answer cheap, and every operator the rank strand emits is over
/// one.
///
/// **The cost is `q^k`, and that is why matrix sparsification being NP-hard is
/// not contradicted.** The hardness is in the regime where the column space
/// cannot be enumerated; a `13x5` operator over GF(2) has 31 nonzero vectors in
/// its column space and a `19x6` has 63. `require_room` prices the walk before it
/// is taken, so an operator too wide for this says so instead of dying.
namespace matrix_sparsification {

/// A basis of the column space of `given` of least total weight, as a matrix of
/// the same shape.
template <class Field>
linear_algebra::MatrixOver<Field> sparsest_basis_over_a_finite_field(
    const Field& field, const linear_algebra::MatrixOver<Field>& given) {
    using Matrix = linear_algebra::MatrixOver<Field>;
    const std::size_t rows = given.rows();
    const std::size_t width = given.columns();
    if (width == 0 || rows == 0) return given;

    const std::size_t modulus = static_cast<std::size_t>(field.residu());
    std::size_t count = 1;
    for (std::size_t taken = 0; taken < width; ++taken) {
        run_limits::require_room("the operator's column space", count,
                                    rows * sizeof(typename Field::Element));
        count *= modulus;
    }

    // Every element of the column space, with its weight. Index `code` is the
    // coefficient tuple read in base `modulus`, so 0 is the zero vector and is
    // skipped: it is in every span and can never enlarge one.
    std::vector<std::pair<std::size_t, std::size_t>> by_weight;  // weight, code
    by_weight.reserve(count - 1);
    std::vector<typename Field::Element> column(rows);
    for (std::size_t code = 1; code < count; ++code) {
        for (std::size_t row = 0; row < rows; ++row) field.assign(column[row], field.zero);
        std::size_t left = code;
        for (std::size_t taken = 0; taken < width; ++taken) {
            const std::size_t coefficient = left % modulus;
            left /= modulus;
            if (coefficient == 0) continue;
            typename Field::Element scalar;
            field.init(scalar, static_cast<int64_t>(coefficient));
            for (std::size_t row = 0; row < rows; ++row) {
                field.axpyin(column[row], scalar, given(row, taken));
            }
        }
        std::size_t weight = 0;
        for (std::size_t row = 0; row < rows; ++row) {
            if (!field.isZero(column[row])) ++weight;
        }
        by_weight.emplace_back(weight, code);
    }
    std::sort(by_weight.begin(), by_weight.end());

    // The greedy. `chosen` holds the basis so far as columns; a candidate is kept
    // exactly when it raises the rank, which is the matroid's independence test.
    Matrix chosen(rows, width);
    std::size_t taken = 0;
    for (const auto& [weight, code] : by_weight) {
        (void)weight;
        for (std::size_t row = 0; row < rows; ++row) field.assign(column[row], field.zero);
        std::size_t left = code;
        for (std::size_t source = 0; source < width; ++source) {
            const std::size_t coefficient = left % modulus;
            left /= modulus;
            if (coefficient == 0) continue;
            typename Field::Element scalar;
            field.init(scalar, static_cast<int64_t>(coefficient));
            for (std::size_t row = 0; row < rows; ++row) {
                field.axpyin(column[row], scalar, given(row, source));
            }
        }
        Matrix trial(rows, taken + 1);
        for (std::size_t column_index = 0; column_index < taken; ++column_index) {
            for (std::size_t row = 0; row < rows; ++row) {
                trial(row, column_index) = chosen(row, column_index);
            }
        }
        for (std::size_t row = 0; row < rows; ++row) trial(row, taken) = column[row];
        if (linear_algebra::rank(field, trial) != taken + 1) continue;
        for (std::size_t row = 0; row < rows; ++row) chosen(row, taken) = column[row];
        if (++taken == width) break;
    }
    return chosen;
}

}  // namespace matrix_sparsification
