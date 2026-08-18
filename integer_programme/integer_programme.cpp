#include "integer_programme.h"

#include "whole_numbers.h"

namespace optimisation {

namespace {

bool holds(Relation relation, const Number& left, const Number& right) {
    switch (relation) {
        case Relation::LessOrEqual: return left <= right;
        case Relation::GreaterOrEqual: return left >= right;
        case Relation::Equal: return left == right;
    }
    return false;
}

}  // namespace

Constraint constraint_of(const std::vector<Number>& coefficients, Relation relation, Number bound) {
    Constraint constraint;
    constraint.relation = relation;
    constraint.bound = bound;
    for (std::size_t index = 0; index < coefficients.size(); ++index) {
        if (coefficients[index] != Number(0)) constraint.terms.push_back({index, coefficients[index]});
    }
    return constraint;
}

bool satisfies(const IntegerProgramme& programme, const std::vector<Number>& values) {
    if (values.size() != programme.variables.size()) return false;

    for (std::size_t index = 0; index < values.size(); ++index) {
        const Variable& variable = programme.variables[index];
        if (variable.bounded_below && values[index] < variable.lower) return false;
        if (variable.bounded_above && values[index] > variable.upper) return false;
        if (variable.integral && !is_whole(values[index])) return false;
    }

    for (const Constraint& constraint : programme.constraints) {
        Number total = Number(0);
        for (const Coefficient& term : constraint.terms) {
            if (term.variable >= values.size()) return false;
            total += term.value * values[term.variable];
        }
        if (!holds(constraint.relation, total, constraint.bound)) return false;
    }
    return true;
}

Number objective_at(const IntegerProgramme& programme, const std::vector<Number>& values) {
    Number total = Number(0);
    for (std::size_t index = 0; index < values.size() && index < programme.objective.size();
         ++index) {
        total += programme.objective[index] * values[index];
    }
    return total;
}

bool improves(Sense sense, const Number& candidate, const Number& incumbent) {
    return sense == Sense::Minimise ? candidate < incumbent : candidate > incumbent;
}

}  // namespace optimisation
