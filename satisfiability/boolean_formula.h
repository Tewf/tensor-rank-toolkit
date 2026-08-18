#pragma once

#include <cstddef>
#include <vector>

/// A formula in conjunctive normal form, and what it takes to satisfy one.
///
/// Deliberately the smallest thing the reduction needs. It is not a SAT solver
/// and it is not a general CNF library: it holds a formula, evaluates it under
/// an assignment, and, for formulas small enough that `2^n` is nothing,
/// finds a satisfying assignment by trying all of them. That last one exists so
/// the tests can build a witness without depending on a solver being installed.
namespace satisfiability {

/// A variable index and whether it appears negated.
struct Literal {
    std::size_t variable = 0;
    bool negated = false;

    bool holds_under(const std::vector<bool>& assignment) const {
        return assignment[variable] != negated;
    }
};

/// Håstad's construction reads exactly three literals from every clause. A
/// shorter clause is padded by repeating one of its own literals, which changes
/// nothing about what satisfies it.
struct Clause {
    std::vector<Literal> literals;
};

struct Formula {
    std::size_t variable_count = 0;
    std::vector<Clause> clauses;

    std::size_t clause_count() const { return clauses.size(); }
};

/// Whether every clause has a literal that holds.
bool is_satisfied_by(const Formula& formula, const std::vector<bool>& assignment);

/// A satisfying assignment found by trying all `2^n` of them, or an empty
/// optional-by-convention `found = false`.
///
/// Exponential on purpose and refused above `variable_count > 24`: this is for
/// building test witnesses, and anything that wants a real answer should be
/// going through a solver, which is the entire subject of this folder.
struct Assignment {
    bool found = false;
    std::vector<bool> values;
};

Assignment satisfying_assignment(const Formula& formula);

/// The clause with its literals padded to exactly three by repeating the first.
///
/// Throws on an empty clause, which is unsatisfiable and has no first literal
/// to repeat, rather than quietly producing a tensor for a different formula.
Clause padded_to_three_literals(const Clause& clause);

}  // namespace satisfiability
