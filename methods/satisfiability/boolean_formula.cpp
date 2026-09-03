#include "boolean_formula.h"

#include <stdexcept>
#include <string>

namespace satisfiability {

bool is_satisfied_by(const Formula& formula, const std::vector<bool>& assignment) {
    for (const Clause& clause : formula.clauses) {
        bool any = false;
        for (const Literal& literal : clause.literals) {
            any = any || literal.holds_under(assignment);
        }
        if (!any) return false;
    }
    return true;
}

Assignment satisfying_assignment(const Formula& formula) {
    if (formula.variable_count > 24) {
        throw std::invalid_argument("trying every assignment of " +
                                    std::to_string(formula.variable_count) +
                                    " variables is not a plan; use a solver");
    }

    const std::size_t total = std::size_t{1} << formula.variable_count;
    std::vector<bool> values(formula.variable_count, false);
    for (std::size_t candidate = 0; candidate < total; ++candidate) {
        for (std::size_t variable = 0; variable < formula.variable_count; ++variable) {
            values[variable] = ((candidate >> variable) & 1u) != 0;
        }
        if (is_satisfied_by(formula, values)) return Assignment{true, values};
    }
    return Assignment();
}

Clause padded_to_three_literals(const Clause& clause) {
    if (clause.literals.empty()) {
        throw std::invalid_argument("an empty clause cannot be padded, and cannot be satisfied");
    }
    Clause padded = clause;
    while (padded.literals.size() < 3) padded.literals.push_back(padded.literals.front());
    if (padded.literals.size() > 3) {
        throw std::invalid_argument("this is a reduction from 3SAT, but a clause has " +
                                    std::to_string(padded.literals.size()) + " literals");
    }
    return padded;
}

}  // namespace satisfiability
