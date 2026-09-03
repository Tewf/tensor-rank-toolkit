#include "pattern_feasibility.h"

#include <stdexcept>
#include <string>

#include "matrix_ops.h"
#include "measures.h"
#include "solver.h"

namespace matrix_sparsification {

std::size_t maximum_pinned_order() { return 8; }

namespace {

/// The unknowns, one per free entry, in row-major order over the pinned block.
using VariableIndex = std::vector<std::vector<std::size_t>>;

constexpr std::size_t kNotFree = static_cast<std::size_t>(-1);

VariableIndex number_the_unknowns(const std::vector<std::vector<bool>>& free_entries,
                                  std::size_t& count) {
    VariableIndex numbered(free_entries.size());
    count = 0;
    for (std::size_t row = 0; row < free_entries.size(); ++row) {
        numbered[row].assign(free_entries[row].size(), kNotFree);
        for (std::size_t column = 0; column < free_entries[row].size(); ++column) {
            if (free_entries[row][column]) numbered[row][column] = count++;
        }
    }
    return numbered;
}

/// `left * right`, where the left is scalar and the right is polynomial.
PolynomialMatrix apply(const Field& field, const Matrix& left, const PolynomialMatrix& right) {
    PolynomialMatrix product(left.rows(), std::vector<Polynomial>(right[0].size()));
    for (std::size_t row = 0; row < left.rows(); ++row) {
        for (std::size_t column = 0; column < right[0].size(); ++column) {
            for (std::size_t inner = 0; inner < right.size(); ++inner) {
                if (field.isZero(left(row, inner))) continue;
                const Polynomial scaled = right[inner][column].multiplied_by(
                    field, Polynomial::constant(field, left(row, inner)));
                product[row][column].add_in(field, scaled);
            }
        }
    }
    return product;
}

Element evaluate(const Field& field, const Polynomial& polynomial,
                 const std::vector<Element>& point) {
    Element total;
    field.assign(total, field.zero);
    for (const auto& term : polynomial.terms()) {
        Element value = term.second;
        for (std::size_t variable : term.first) field.mulin(value, point[variable]);
        field.addin(total, value);
    }
    return total;
}

/// Walk the grid `{0, 1, ..., degree}^unknowns` for a point where the
/// obstruction does not vanish.
///
/// A nonzero polynomial of total degree `d` cannot vanish on every point of a
/// grid with `d + 1` values per variable, so on this grid a miss is a proof and
/// not a shrug, so long as the grid is small enough to finish, which is why the
/// caller is told when it was not.
bool find_witness_point(const Field& field, const Polynomial& obstruction, std::size_t unknowns,
                        std::size_t budget, std::vector<Element>& point) {
    const std::size_t values = obstruction.degree() + 1;

    std::size_t total = 1;
    for (std::size_t variable = 0; variable < unknowns; ++variable) {
        if (total > budget / values) return false;
        total *= values;
    }

    std::vector<std::size_t> digits(unknowns, 0);
    for (std::size_t visited = 0; visited < total; ++visited) {
        point.assign(unknowns, Element());
        for (std::size_t variable = 0; variable < unknowns; ++variable) {
            field.init(point[variable], static_cast<int64_t>(digits[variable]));
        }
        if (!field.isZero(evaluate(field, obstruction, point))) return true;

        for (std::size_t variable = 0; variable < unknowns; ++variable) {
            if (++digits[variable] < values) break;
            digits[variable] = 0;
        }
    }
    return false;
}

}  // namespace

PatternVerdict decide_pattern(const Field& field, const Matrix& operator_,
                              const std::vector<std::size_t>& pinned_rows,
                              const std::vector<std::vector<bool>>& free_entries) {
    const std::size_t order = operator_.columns();
    if (pinned_rows.size() != order || free_entries.size() != order) {
        throw std::invalid_argument("pinned rows and pattern must both be " +
                                    std::to_string(order) + " to match the operator's columns");
    }
    if (order > maximum_pinned_order()) {
        throw std::invalid_argument("expanding a determinant of order " + std::to_string(order) +
                                    " is factorial; the ceiling is " +
                                    std::to_string(maximum_pinned_order()));
    }

    Matrix pinned_inverse;
    if (!linear_algebra::invert(field, linear_algebra::select_rows<Field>(operator_, pinned_rows),
                                pinned_inverse)) {
        throw std::invalid_argument("the pinned rows are singular, so they fix no change of basis");
    }

    std::size_t unknowns = 0;
    const VariableIndex numbered = number_the_unknowns(free_entries, unknowns);

    PolynomialMatrix target(order, std::vector<Polynomial>(order));
    for (std::size_t row = 0; row < order; ++row) {
        for (std::size_t column = 0; column < order; ++column) {
            if (numbered[row][column] == kNotFree) continue;
            target[row][column] = Polynomial::variable(field, numbered[row][column]);
        }
    }

    // P = (Q L)^-1 (Q A): the change of basis the pinned rows force.
    const PolynomialMatrix basis_change = apply(field, pinned_inverse, target);

    PatternVerdict verdict;
    verdict.obstruction = determinant(field, basis_change);
    verdict.realisable = !verdict.obstruction.is_zero();

    const PolynomialMatrix under_it = apply(field, operator_, basis_change);
    for (const std::vector<Polynomial>& row : under_it) {
        for (const Polynomial& entry : row) {
            if (entry.is_zero()) ++verdict.guaranteed_zeros;
        }
    }
    if (!verdict.realisable) return verdict;

    std::vector<Element> point;
    if (!find_witness_point(field, verdict.obstruction, unknowns, 1u << 20, point)) return verdict;

    verdict.witnessed = true;
    verdict.change_of_basis = Matrix(order, order);
    for (std::size_t row = 0; row < order; ++row) {
        for (std::size_t column = 0; column < order; ++column) {
            verdict.change_of_basis(row, column) = evaluate(field, basis_change[row][column], point);
        }
    }
    verdict.result = linear_algebra::multiply(field, operator_, verdict.change_of_basis);
    return verdict;
}

}  // namespace matrix_sparsification
