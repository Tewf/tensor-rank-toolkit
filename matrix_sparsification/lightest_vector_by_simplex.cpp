#include "lightest_vector_by_simplex.h"

#include <algorithm>

#include "integer_programme.h"
#include "matrix_ops.h"
#include "measures.h"
#include "simplex.h"
#include "solver.h"
#include "span_basis.h"
#include "span_queries.h"
#include "standard_form.h"

namespace matrix_sparsification {

namespace {

using optimisation::Number;

/// A basis of `{y : rows · y = 0}`, the null space, which is what the programme
/// constrains against.
///
/// A vector lies in the row space exactly when it is orthogonal to every vector
/// of the null space, the two being orthogonal complements over `Q`. So the
/// programme's equality rows are these, not the operator's own.
///
/// `vanishing_combinations` returns the `x` with `Σ x_j · (j-th vector given) = 0`.
/// Hand it the **columns** and that reads `rows · x = 0`, which is the null space
/// wanted; hand it the rows and it would answer a different question, about
/// dependence among the rows themselves.
std::vector<std::vector<Element>> annihilator_of(const Field& field, const Matrix& rows) {
    const Matrix columns = linear_algebra::transpose<Field>(rows);
    std::vector<std::vector<Element>> as_vectors;
    as_vectors.reserve(columns.rows());
    for (std::size_t row = 0; row < columns.rows(); ++row) {
        as_vectors.push_back(columns.row(row));
    }
    return linear_algebra::vanishing_combinations(field, as_vectors);
}

/// `min Σ(u_i + v_i)` subject to `H(u − v) = 0` and `u_j − v_j = 1`.
///
/// The absolute value is not expressible, so each coordinate is split into a
/// non-negative pair and the objective is the sum of both halves; at an optimum
/// at most one of each pair is nonzero, so their difference is the vector and
/// the objective is its weight in the `ℓ1` sense.
optimisation::IntegerProgramme programme_for(
    const Field& field, const std::vector<std::vector<Element>>& annihilator,
    std::size_t width, std::size_t pinned) {
    optimisation::IntegerProgramme model;
    model.sense = optimisation::Sense::Minimise;
    model.variables.resize(2 * width);
    model.objective.assign(2 * width, Number(1));
    for (std::size_t index = 0; index < 2 * width; ++index) {
        // Both bounds stated rather than left to a format's default, which is
        // what ../integer_programme/README.md asks of every caller.
        model.variables[index].name = (index < width ? "u" : "v") +
                                      std::to_string(index % width + 1);
        model.variables[index].bounded_below = true;
        model.variables[index].lower = Number(0);
        model.variables[index].bounded_above = false;
    }
    for (const std::vector<Element>& row : annihilator) {
        optimisation::Constraint constraint;
        constraint.relation = optimisation::Relation::Equal;
        constraint.bound = Number(0);
        for (std::size_t column = 0; column < width; ++column) {
            if (field.isZero(row[column])) continue;
            Element negated;
            field.neg(negated, row[column]);
            constraint.terms.push_back({column, row[column]});
            constraint.terms.push_back({column + width, negated});
        }
        if (!constraint.terms.empty()) model.constraints.push_back(std::move(constraint));
    }
    optimisation::Constraint pin;
    pin.relation = optimisation::Relation::Equal;
    pin.bound = Number(1);
    pin.terms.push_back({pinned, Number(1)});
    pin.terms.push_back({pinned + width, Number(-1)});
    model.constraints.push_back(std::move(pin));
    return model;
}

}  // namespace

LightestVectors lightest_vectors_by_simplex(const Field& field, const Matrix& rows) {
    LightestVectors answer;
    const std::size_t width = rows.columns();
    answer.basis = Matrix(rows.rows(), width);
    if (rows.rows() == 0 || width == 0) return answer;

    linear_algebra::SpanBasis<Field> space(field, width);
    for (std::size_t row = 0; row < rows.rows(); ++row) {
        space.try_add(rows.data() + row * width, width);
    }
    const std::size_t dimension = space.dimension();
    const std::vector<std::vector<Element>> annihilator = annihilator_of(field, rows);

    std::vector<std::pair<std::size_t, std::vector<Element>>> found;
    for (std::size_t pinned = 0; pinned < width; ++pinned) {
        const optimisation::StandardForm form =
            optimisation::standard_form_of(programme_for(field, annihilator, width, pinned));
        const optimisation::LinearOptimum optimum = optimisation::solve_relaxation(form);
        if (optimum.status != optimisation::Status::Optimal) continue;  // that coordinate carries none
        const std::vector<Number> point = optimisation::original_point(form, optimum.values);
        std::vector<Element> vector(width, Element());
        std::size_t weight = 0;
        for (std::size_t column = 0; column < width; ++column) {
            // Exact, so a coordinate is zero because it is, not because it is
            // under a threshold. The prototype in Python needed one; this does not.
            Element value;
            field.sub(value, point[column], point[column + width]);
            if (field.isZero(value)) continue;
            vector[column] = value;
            ++weight;
        }
        if (weight != 0) found.emplace_back(weight, std::move(vector));
    }
    if (found.empty()) return answer;

    std::stable_sort(found.begin(), found.end(),
                     [](const auto& left, const auto& right) { return left.first < right.first; });
    answer.least = found.front().first;

    linear_algebra::SpanBasis<Field> held(field, width);
    for (const auto& [weight, vector] : found) {
        if (!held.try_add(vector)) continue;
        for (std::size_t column = 0; column < width; ++column) {
            answer.basis(held.dimension() - 1, column) = vector[column];
        }
        answer.weights.push_back(weight);
        if (held.dimension() == dimension) break;
    }
    // Sparsity is trivial to improve by returning something else, so whether it
    // is the same space is checked here and not left to the caller to assume.
    answer.spans = held.dimension() == dimension &&
                   linear_algebra::same_row_space(field, rows, answer.basis);
    return answer;
}

}  // namespace matrix_sparsification
