#include "standard_form.h"

#include <limits>

namespace integer_programme {

namespace {

constexpr std::size_t kNoSlack = std::numeric_limits<std::size_t>::max();

/// The constraints and the upper bounds in one list: once the simplex below
/// knows only `x ≥ 0`, an upper bound is a row like any other.
struct Row {
    std::vector<Coefficient> terms;
    Relation relation;
    Number bound;
};

std::vector<Row> rows_of(const IntegerProgramme& programme) {
    const std::size_t width = programme.variables.size();
    std::vector<Row> rows;
    for (const Constraint& constraint : programme.constraints) {
        rows.push_back({constraint.terms, constraint.relation, constraint.bound});
    }
    for (std::size_t index = 0; index < width; ++index) {
        if (!programme.variables[index].bounded_above) continue;
        rows.push_back({{Coefficient{index, Number(1)}}, Relation::LessOrEqual,
                        programme.variables[index].upper});
    }
    return rows;
}

}  // namespace

StandardForm standard_form_of(const IntegerProgramme& programme) {
    const std::size_t width = programme.variables.size();
    const std::vector<Row> rows = rows_of(programme);

    StandardForm form;
    form.origin.resize(width);
    for (std::size_t index = 0; index < width; ++index) {
        const Variable& variable = programme.variables[index];
        form.origin[index].positive = form.columns++;
        form.origin[index].split = !variable.bounded_below;
        if (form.origin[index].split) {
            form.origin[index].negative = form.columns++;
        } else {
            form.origin[index].shift = variable.lower;
        }
    }

    std::vector<std::size_t> slack(rows.size(), kNoSlack);
    for (std::size_t row = 0; row < rows.size(); ++row) {
        if (rows[row].relation != Relation::Equal) slack[row] = form.columns++;
    }

    form.rows.assign(rows.size(), std::vector<Number>(form.columns, Number(0)));
    form.bound.assign(rows.size(), Number(0));
    for (std::size_t row = 0; row < rows.size(); ++row) {
        Number shifted = rows[row].bound;
        for (const Coefficient& term : rows[row].terms) {
            if (term.variable >= width) continue;
            const StandardForm::Origin& origin = form.origin[term.variable];
            form.rows[row][origin.positive] += term.value;
            if (origin.split) {
                form.rows[row][origin.negative] -= term.value;
            } else {
                shifted -= term.value * origin.shift;
            }
        }
        if (slack[row] != kNoSlack) {
            form.rows[row][slack[row]] =
                rows[row].relation == Relation::LessOrEqual ? Number(1) : Number(-1);
        }
        // Phase one wants a feasible basis of artificials, which needs `b ≥ 0`.
        // Negating a whole equality changes nothing it says.
        if (shifted < Number(0)) {
            for (Number& entry : form.rows[row]) entry = Number(0) - entry;
            shifted = Number(0) - shifted;
        }
        form.bound[row] = shifted;
    }

    form.cost.assign(form.columns, Number(0));
    for (std::size_t index = 0; index < width && index < programme.objective.size(); ++index) {
        const Number coefficient = programme.sense == Sense::Minimise
                                       ? programme.objective[index]
                                       : Number(0) - programme.objective[index];
        form.cost[form.origin[index].positive] += coefficient;
        if (form.origin[index].split) form.cost[form.origin[index].negative] -= coefficient;
    }
    return form;
}

std::vector<Number> original_point(const StandardForm& form, const std::vector<Number>& values) {
    std::vector<Number> point(form.origin.size(), Number(0));
    for (std::size_t index = 0; index < form.origin.size(); ++index) {
        const StandardForm::Origin& origin = form.origin[index];
        if (origin.positive >= values.size()) continue;
        point[index] = origin.shift + values[origin.positive];
        if (origin.split && origin.negative < values.size()) point[index] -= values[origin.negative];
    }
    return point;
}

}  // namespace integer_programme
