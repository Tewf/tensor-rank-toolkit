/// That the simplex route agrees with the search, where the search can answer.
///
/// The route is only *optimal* when the operator's matroid is regular, which is
/// a property of the operator and not of this code. So the check that matters is
/// the one that holds either way: **whatever it returns is a basis of the same
/// space**, and where the exhaustive scan finishes, the two agree on the least
/// weight.
///
/// The prototypes this replaces are in [`../prototypes/`](../prototypes/)
/// and the finding is [`../method/answering-without-searching.md`](../method/answering-without-searching.md).
#include <cstddef>
#include <string>
#include <vector>

#include "check.h"
#include "lightest_vector_by_simplex.h"
#include "linear_algebra.h"
#include "rational_sparsifier.h"
#include "span_queries.h"

namespace {

using matrix_sparsification::Field;
using matrix_sparsification::Matrix;

Matrix matrix_of(const Field& field, std::size_t rows, std::size_t columns,
                 const std::vector<int64_t>& entries) {
    Matrix built(rows, columns);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            field.init(built(row, column), entries[row * columns + column]);
        }
    }
    return built;
}

/// Both routes on one operator: the same space, and the same least weight.
void agree_on(const std::string& name, const Field& field, const Matrix& given) {
    const matrix_sparsification::LightestVectors byLp =
        matrix_sparsification::lightest_vectors_by_simplex(field, given);
    const Matrix byScan = matrix_sparsification::sparsest_basis_over_the_rationals(field, given);

    check::equal(name + ": the simplex route spans the operator", byLp.spans, 1);
    check::equal(name + ": and it is the same row space",
                 linear_algebra::same_row_space(field, given, byLp.basis), 1);

    // The scan walks weights upward, so its lightest vector is the true minimum.
    std::size_t lightest = 0;
    for (std::size_t row = 0; row < byScan.rows(); ++row) {
        std::size_t weight = 0;
        for (std::size_t column = 0; column < byScan.columns(); ++column) {
            if (!field.isZero(byScan(row, column))) ++weight;
        }
        if (weight != 0 && (lightest == 0 || weight < lightest)) lightest = weight;
    }
    check::equal(name + ": least weight matches the exhaustive scan",
                 static_cast<long long>(byLp.least), static_cast<long long>(lightest));
}

}  // namespace

int main() {
    const Field field;

    // A pair whose difference is lighter than either, which is the whole of the
    // question in one case.
    agree_on("a pair whose difference is lighter", field,
             matrix_of(field, 2, 4, {1, 1, 1, 0, 1, 1, 0, 1}));

    // Two rows sharing a zero set, so the vanishing space there has dimension
    // two. This is the shape that separates a careless search from a correct one.
    agree_on("a corank-two zero set", field,
             matrix_of(field, 3, 5, {1, 1, 0, 0, 0, 1, -1, 0, 0, 0, 1, 0, 1, 1, 1}));

    // Entries that are neither 0 nor +-1, so the vertices are genuinely rational
    // and an inexact solver would round them. This is the case the external
    // solver chain cannot do, which is why the built-in simplex is called here.
    agree_on("rational entries", field,
             matrix_of(field, 3, 5, {2, 3, 0, 1, 0, 4, 6, 1, 0, 0, 1, 0, 0, 2, 3}));

    // Wider than tall by a margin, where most coordinates carry no light vector
    // and the programme comes back infeasible for them.
    agree_on("a wide operator", field,
             matrix_of(field, 3, 7, {1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0,
                                     1, 1, 0, 1, 1, 0, 1}));

    // Rank deficient: the third row is the sum of the first two, so a basis has
    // two vectors and the route must say it spans rather than claim three.
    const Matrix deficient = matrix_of(field, 3, 4, {1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1});
    const matrix_sparsification::LightestVectors thin =
        matrix_sparsification::lightest_vectors_by_simplex(field, deficient);
    check::equal("rank deficient: spans what it was given", thin.spans, 1);
    check::equal("rank deficient: as many vectors as the rank",
                 static_cast<long long>(thin.weights.size()), 2);

    return check::report("lightest_vector_by_simplex");
}
