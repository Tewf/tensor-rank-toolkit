#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "field.h"
#include "matrix.h"

/// Solving an exact linear system, and the one thing built on it: inversion.
///
/// Every operation goes through the field, which ensures exact arithmetic and
/// coherence across all computations.
namespace linear_algebra {

/// Solve `coefficients * rows = target` for rows that are linearly independent,
/// so the solution is unique when it exists. False when `target` is outside
/// their span.
template <class Field>
bool solve_in_row_space(const Field& field,
                        const std::vector<std::vector<typename Field::Element>>& rows,
                        const std::vector<typename Field::Element>& target,
                        std::vector<typename Field::Element>& coefficients) {
    using Element = typename Field::Element;
    const std::size_t unknowns = rows.size();
    if (unknowns == 0) {
        coefficients.clear();
        for (const Element& entry : target) {
            if (!field.isZero(entry)) return false;
        }
        return true;
    }
    const std::size_t equations = target.size();

    // Augmented system: column j is rows[j] read down the equations, and the
    // last column is the target.
    MatrixOver<Field> system(equations, unknowns + 1);
    for (std::size_t equation = 0; equation < equations; ++equation) {
        for (std::size_t unknown = 0; unknown < unknowns; ++unknown) {
            system(equation, unknown) = rows[unknown][equation];
        }
        system(equation, unknowns) = target[equation];
    }

    std::vector<std::size_t> pivot_of_unknown(unknowns, equations);  // equations means "none"
    std::size_t pivot_row = 0;
    for (std::size_t unknown = 0; unknown < unknowns && pivot_row < equations; ++unknown) {
        std::size_t found = equations;
        for (std::size_t row = pivot_row; row < equations; ++row) {
            if (!field.isZero(system(row, unknown))) {
                found = row;
                break;
            }
        }
        if (found == equations) continue;

        for (std::size_t column = 0; column <= unknowns; ++column) {
            std::swap(system(pivot_row, column), system(found, column));
        }
        Element scale;
        field.inv(scale, system(pivot_row, unknown));
        for (std::size_t column = 0; column <= unknowns; ++column) {
            field.mulin(system(pivot_row, column), scale);
        }
        for (std::size_t row = 0; row < equations; ++row) {
            if (row == pivot_row || field.isZero(system(row, unknown))) continue;
            Element factor;
            field.neg(factor, system(row, unknown));
            for (std::size_t column = 0; column <= unknowns; ++column) {
                field.axpyin(system(row, column), factor, system(pivot_row, column));
            }
        }
        pivot_of_unknown[unknown] = pivot_row;
        ++pivot_row;
    }

    // An equation left reading 0 == nonzero means the target is outside the span.
    for (std::size_t row = pivot_row; row < equations; ++row) {
        if (!field.isZero(system(row, unknowns))) return false;
    }

    coefficients.assign(unknowns, Element());
    for (std::size_t unknown = 0; unknown < unknowns; ++unknown) {
        if (pivot_of_unknown[unknown] != equations) {
            coefficients[unknown] = system(pivot_of_unknown[unknown], unknowns);
        }
    }
    return true;
}

/// A basis of `{x : x . rows == 0}`, the combinations of `rows` that vanish.
///
/// The homogeneous half of the solver above. `solve_in_row_space` answers "which
/// combination reaches this target", and needs the rows independent to have one
/// answer; this one answers "which combinations reach nothing", which is the
/// question exactly when they are *not* independent, and there the answer is a
/// space rather than a vector.
///
/// Eliminates on `[rows | I]`, so the transform is carried alongside rather than
/// recomputed: a row whose left half has been cleared to zero has its left null
/// vector sitting in the right half. There is one unknown per row, and the width
/// is the rows' own, not a second argument that could disagree with them: rows
/// of no width are the system with no equations, whose answer is everything.
template <class Field>
std::vector<std::vector<typename Field::Element>> vanishing_combinations(
    const Field& field, const std::vector<std::vector<typename Field::Element>>& rows) {
    using Element = typename Field::Element;
    const std::size_t unknowns = rows.size();
    const std::size_t width = unknowns == 0 ? 0 : rows.front().size();
    std::vector<std::vector<Element>> carried(unknowns, std::vector<Element>(unknowns, Element()));
    std::vector<std::vector<Element>> working = rows;
    for (std::size_t row = 0; row < unknowns; ++row) field.assign(carried[row][row], field.one);

    std::size_t pivot_row = 0;
    for (std::size_t column = 0; column < width && pivot_row < unknowns; ++column) {
        std::size_t found = unknowns;
        for (std::size_t row = pivot_row; row < unknowns; ++row) {
            if (!field.isZero(working[row][column])) {
                found = row;
                break;
            }
        }
        if (found == unknowns) continue;
        std::swap(working[pivot_row], working[found]);
        std::swap(carried[pivot_row], carried[found]);
        for (std::size_t row = pivot_row + 1; row < unknowns; ++row) {
            if (field.isZero(working[row][column])) continue;
            Element factor;
            field.div(factor, working[row][column], working[pivot_row][column]);
            field.negin(factor);
            for (std::size_t at = 0; at < width; ++at) {
                field.axpyin(working[row][at], factor, working[pivot_row][at]);
            }
            for (std::size_t at = 0; at < unknowns; ++at) {
                field.axpyin(carried[row][at], factor, carried[pivot_row][at]);
            }
        }
        ++pivot_row;
    }

    // Every row past the last pivot is a combination of the originals that came
    // out zero, and they are independent because the transform is invertible.
    return std::vector<std::vector<Element>>(carried.begin() + static_cast<std::ptrdiff_t>(pivot_row),
                                             carried.end());
}

/// Exact inverse of a square matrix, false if it is singular.
///
/// Row j of the inverse is the combination of the rows of `square` that makes
/// the j-th standard basis vector, so this is the solver above run once per
/// row rather than a second elimination that could disagree with it.
template <class Field>
bool invert(const Field& field, const MatrixOver<Field>& square, MatrixOver<Field>& inverse) {
    using Element = typename Field::Element;
    const std::size_t order = square.rows();
    if (order != square.columns()) return false;

    std::vector<std::vector<Element>> rows;
    rows.reserve(order);
    for (std::size_t row = 0; row < order; ++row) rows.push_back(square.row(row));

    inverse = MatrixOver<Field>(order, order);
    for (std::size_t index = 0; index < order; ++index) {
        std::vector<Element> target(order, Element());
        field.assign(target[index], field.one);
        std::vector<Element> coefficients;
        if (!solve_in_row_space(field, rows, target, coefficients)) return false;
        for (std::size_t column = 0; column < order; ++column) {
            inverse(index, column) = coefficients[column];
        }
    }
    return true;
}

}  // namespace linear_algebra
