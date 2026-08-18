#include "heuristic_sparsifier.h"

#include "combinations.h"
#include "matrix_ops.h"
#include "measures.h"
#include "solver.h"

namespace matrix_sparsification {

Matrix row_basis_sparsifier(const Field& field, const Matrix& u) {
    const std::size_t order = u.columns();

    Matrix best(order, order);
    for (std::size_t index = 0; index < order; ++index) field.assign(best(index, index), field.one);
    std::size_t fewest = linear_algebra::nonzero_count(field, u);

    for (const std::vector<std::size_t>& chosen : combinations(u.rows(), order)) {
        const Matrix block = linear_algebra::select_rows<Field>(u, chosen);
        Matrix inverse;
        if (!linear_algebra::invert(field, block, inverse)) continue;

        const std::size_t count =
            linear_algebra::nonzero_count(field, linear_algebra::multiply(field, u, inverse));
        if (count < fewest) {
            fewest = count;
            best = inverse;
        }
    }
    return best;
}

}  // namespace matrix_sparsification
