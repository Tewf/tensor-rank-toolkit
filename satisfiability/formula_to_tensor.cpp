#include "formula_to_tensor.h"

#include <stdexcept>

#include "decomposition.h"
#include "measures.h"

namespace satisfiability {

namespace {

std::size_t row_count(const Formula& formula) {
    return 2 + formula.variable_count + 2 * formula.clause_count();
}

std::size_t column_count(const Formula& formula) { return 3 * formula.variable_count; }

Matrix empty_slice(const Formula& formula) {
    return Matrix(row_count(formula), column_count(formula));
}

/// Add the literal's vector into `row` of `slice`, scaled by `sign`.
///
/// `x_v` is a single 1 at column `2v`; `¬x_v` is 1 at `2v` and at `2v+1`. The
/// sign is what makes `u₁ − u₂` a subtraction rather than a second addition.
void add_literal(const Field& field, Matrix& slice, std::size_t row, const Literal& literal,
                 bool subtract) {
    Element unit;
    if (subtract) {
        field.neg(unit, field.one);
    } else {
        field.assign(unit, field.one);
    }

    field.addin(slice(row, 2 * literal.variable), unit);
    if (literal.negated) field.addin(slice(row, 2 * literal.variable + 1), unit);
}

Matrix variable_slice(const Field& field, const Formula& formula, std::size_t variable) {
    Matrix slice = empty_slice(formula);
    field.assign(slice(0, 2 * variable), field.one);
    field.assign(slice(1, 2 * variable + 1), field.one);
    return slice;
}

Matrix support_slice(const Field& field, const Formula& formula, std::size_t variable) {
    Matrix slice = empty_slice(formula);
    field.assign(slice(0, 2 * formula.variable_count + variable), field.one);
    return slice;
}

Matrix marker_slice(const Field& field, const Formula& formula, std::size_t variable) {
    Matrix slice = empty_slice(formula);
    field.assign(slice(0, 2 * variable), field.one);
    field.assign(slice(variable + 2, 2 * variable + 1), field.one);
    field.assign(slice(variable + 2, 2 * formula.variable_count + variable), field.one);
    return slice;
}

Matrix clause_slice(const Field& field, const Formula& formula, std::size_t index) {
    const Clause clause = padded_to_three(formula.clauses[index]);
    const std::size_t first = formula.variable_count + 2 + 2 * index;

    Matrix slice = empty_slice(formula);
    add_literal(field, slice, 0, clause.literals[0], false);
    for (std::size_t offset = 0; offset < 2; ++offset) {
        add_literal(field, slice, first + offset, clause.literals[0], false);
        add_literal(field, slice, first + offset, clause.literals[offset + 1], true);
    }
    return slice;
}

Matrix difference(const Field& field, const Matrix& left, const Matrix& right) {
    Matrix result = left;
    for (std::size_t entry = 0; entry < result.entry_count(); ++entry) {
        field.subin(result.data()[entry], right.data()[entry]);
    }
    return result;
}

/// `V_v⁽¹⁾`: the literal the assignment makes true, in the first row alone.
Matrix satisfied_literal_slice(const Field& field, const Formula& formula, std::size_t variable,
                               bool value) {
    Matrix slice = empty_slice(formula);
    add_literal(field, slice, 0, Literal{variable, !value}, false);
    return slice;
}

}  // namespace

std::size_t target_rank(const Formula& formula) {
    return 4 * formula.variable_count + 2 * formula.clause_count();
}

linear_algebra::Tensor formula_to_tensor(const Field& field, const Formula& formula) {
    linear_algebra::Tensor tensor;
    tensor.characteristic = static_cast<int64_t>(field.characteristic());

    for (std::size_t variable = 0; variable < formula.variable_count; ++variable) {
        tensor.slices.push_back(variable_slice(field, formula, variable));
    }
    for (std::size_t variable = 0; variable < formula.variable_count; ++variable) {
        tensor.slices.push_back(support_slice(field, formula, variable));
    }
    for (std::size_t variable = 0; variable < formula.variable_count; ++variable) {
        tensor.slices.push_back(marker_slice(field, formula, variable));
    }
    for (std::size_t index = 0; index < formula.clause_count(); ++index) {
        tensor.slices.push_back(clause_slice(field, formula, index));
    }
    return tensor;
}

std::vector<Matrix> witness_from_assignment(const Field& field, const Formula& formula,
                                            const std::vector<bool>& assignment) {
    if (!is_satisfied_by(formula, assignment)) {
        throw std::invalid_argument("that assignment does not satisfy the formula, so Lemma 2 "
                                    "builds no witness from it");
    }

    std::vector<Matrix> witness;
    std::vector<Matrix> satisfied(formula.variable_count);
    for (std::size_t variable = 0; variable < formula.variable_count; ++variable) {
        satisfied[variable] =
            satisfied_literal_slice(field, formula, variable, assignment[variable]);

        const Matrix whole = variable_slice(field, formula, variable);
        witness.push_back(satisfied[variable]);
        witness.push_back(difference(field, whole, satisfied[variable]));
        witness.push_back(support_slice(field, formula, variable));

        Matrix marker = difference(field, marker_slice(field, formula, variable), satisfied[variable]);
        if (!assignment[variable]) {
            marker = difference(field, marker, support_slice(field, formula, variable));
        }
        witness.push_back(marker);
    }

    for (std::size_t index = 0; index < formula.clause_count(); ++index) {
        const Clause clause = padded_to_three(formula.clauses[index]);
        std::size_t chosen = clause.literals[0].variable;
        for (const Literal& literal : clause.literals) {
            if (literal.holds_under(assignment)) {
                chosen = literal.variable;
                break;
            }
        }

        const Matrix remainder =
            difference(field, clause_slice(field, formula, index), satisfied[chosen]);
        for (const Matrix& piece : linear_algebra::rank_one_decomposition(field, remainder)) {
            witness.push_back(piece);
        }
    }
    return witness;
}

}  // namespace satisfiability
