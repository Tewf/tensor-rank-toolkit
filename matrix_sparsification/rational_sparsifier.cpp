#include "rational_sparsifier.h"

#include <functional>

#include "combinations.h"
#include "memory_budget.h"
#include "solver.h"
#include "span_basis.h"

namespace matrix_sparsification {

namespace {

using Space = linear_algebra::SpanBasis<Field>;

/// The row space in reduced echelon form, which is what makes a support cheap
/// to test.
///
/// Reduced, every codeword is `x` read off its own pivot columns: entry `j` of
/// the codeword, for `j` a pivot, is the coefficient of that echelon row. So
/// asking for a codeword supported inside `support` fixes the coefficients of
/// every echelon row whose pivot lies outside it to zero, and leaves as many
/// unknowns as there are pivots inside it, never more than `|support|`.
Space echelon_form(const Field& field, const Matrix& rows) {
    Space space(field, rows.columns());
    for (std::size_t row = 0; row < rows.rows(); ++row) {
        space.try_add(rows.data() + row * rows.columns(), rows.columns());
    }
    return space;
}

/// Every codeword supported inside `support`, as a basis of that subspace.
///
/// The unknowns are the echelon rows whose pivot is inside `support`; the
/// equations are the columns that are neither a pivot nor in `support`, each of
/// which the codeword has to leave at zero. A pivot column outside `support` is
/// not an equation, it is an unknown struck out.
std::vector<std::vector<Element>> codewords_supported_in(
    const Field& field, const Space& space, const std::vector<std::size_t>& support,
    const std::vector<char>& is_pivot, std::vector<char>& inside) {
    for (std::size_t column : support) inside[column] = 1;

    std::vector<std::size_t> unknowns;
    for (std::size_t index = 0; index < space.dimension(); ++index) {
        if (inside[space.pivot_columns()[index]]) unknowns.push_back(index);
    }
    std::vector<std::size_t> equations;
    for (std::size_t column = 0; column < inside.size(); ++column) {
        if (!inside[column] && !is_pivot[column]) equations.push_back(column);
    }

    for (std::size_t column : support) inside[column] = 0;
    if (unknowns.empty()) return {};

    // One row per unknown, read down every equation, which is the shape
    // `vanishing_combinations` takes.
    std::vector<std::vector<Element>> by_unknown;
    by_unknown.reserve(unknowns.size());
    for (std::size_t index : unknowns) {
        std::vector<Element> entries;
        entries.reserve(equations.size());
        for (std::size_t column : equations) entries.push_back(space.rows()[index][column]);
        by_unknown.push_back(std::move(entries));
    }

    std::vector<std::vector<Element>> found;
    for (const std::vector<Element>& coefficients :
         linear_algebra::vanishing_combinations(field, by_unknown)) {
        std::vector<Element> codeword(inside.size(), Element());
        for (std::size_t k = 0; k < unknowns.size(); ++k) {
            if (field.isZero(coefficients[k])) continue;
            for (std::size_t column = 0; column < inside.size(); ++column) {
                field.axpyin(codeword[column], coefficients[k], space.rows()[unknowns[k]][column]);
            }
        }
        found.push_back(std::move(codeword));
    }
    return found;
}

}  // namespace

Matrix sparsest_basis_over_the_rationals(const Field& field, const Matrix& rows) {
    const std::size_t width = rows.columns();
    Matrix answer(rows.rows(), width);
    if (rows.rows() == 0 || width == 0) return answer;

    const Space space = echelon_form(field, rows);
    const std::size_t dimension = space.dimension();

    // **Priced before it is walked, because the walk is what runs forever.**
    // The scan stops at the first weight that completes a basis, and how soon
    // that is depends on the answer, so the only honest thing to price up front
    // is the worst case: the widest level it could reach, `C(b, w)` at
    // `w = (b - r + 1) / 2`. On the 23x9 operators of a rank-23 <3,3,3> scheme
    // that is about ten megabytes and passes; on a 49x16 one it is past any
    // budget, which is the right answer and the one nobody got before. Left to
    // itself that operator ran for thirty minutes and said nothing, and the
    // refusal now names the number and where to go instead.
    const std::size_t ceiling = width - dimension + 1;
    bilinear_rank::require_room(
        "the column supports this scan may walk (see "
        "matrix_sparsification/method/when-the-matroid-is-regular.md for the "
        "route that does not walk them)",
        subset_count(width, ceiling / 2 + 1), sizeof(std::size_t) * (ceiling / 2 + 1));
    Space held(field, width);
    std::vector<char> is_pivot(width, 0);
    for (std::size_t pivot : space.pivot_columns()) is_pivot[pivot] = 1;
    std::vector<char> inside(width, 0);

    // Ascending support size is ascending weight, and the greedy is done as soon
    // as it holds a basis. `n - r + 1` is the ceiling proved in the header, so a
    // scan that reaches it and is still short would be a bug, not a wider search.
    for (std::size_t weight = 1; weight <= width - dimension + 1 && held.dimension() < dimension;
         ++weight) {
        walk_combinations(width, weight, [&](const std::vector<std::size_t>& support) {
            for (const std::vector<Element>& codeword :
                 codewords_supported_in(field, space, support, is_pivot, inside)) {
                if (!held.try_add(codeword)) continue;
                for (std::size_t column = 0; column < width; ++column) {
                    answer(held.dimension() - 1, column) = codeword[column];
                }
                if (held.dimension() == dimension) return false;
            }
            return true;
        });
    }
    return answer;
}

}  // namespace matrix_sparsification
