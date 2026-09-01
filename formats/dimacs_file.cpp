#include "dimacs_file.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>

namespace formats {

namespace {

/// Clauses forcing `result` to be `left` xor `right`.
void equate_to_xor(Cnf& formula, int result, int left, int right) {
    formula.add_clause({-result, left, right});
    formula.add_clause({-result, -left, -right});
    formula.add_clause({result, -left, right});
    formula.add_clause({result, left, -right});
}

}  // namespace

Cnf with_parities_expanded(const Cnf& formula) {
    Cnf expanded;
    expanded.variable_count = formula.variable_count;
    expanded.clauses = formula.clauses;

    for (const Cnf::Parity& parity : formula.parities) {
        if (parity.literals.empty()) {
            // Nothing xored is false. Wanting it true is a contradiction, and
            // the empty clause is how DIMACS says so.
            if (parity.value) expanded.add_clause({});
            continue;
        }

        int running = parity.literals.front();
        for (std::size_t index = 1; index < parity.literals.size(); ++index) {
            const int next = expanded.new_variable();
            equate_to_xor(expanded, next, running, parity.literals[index]);
            running = next;
        }
        expanded.add_clause({parity.value ? running : -running});
    }
    return expanded;
}

std::size_t Cnf::total_clause_count(bool native_xor) const {
    if (native_xor) return clauses.size() + parities.size();
    return with_parities_expanded(*this).clauses.size();
}

std::size_t Cnf::total_variable_count(bool native_xor) const {
    if (native_xor) return variable_count;
    return with_parities_expanded(*this).variable_count;
}

void write_dimacs(std::ostream& output, const Cnf& formula, bool native_xor) {
    const Cnf written = native_xor ? formula : with_parities_expanded(formula);

    output << "p cnf " << written.variable_count << ' '
           << (written.clauses.size() + written.parities.size()) << '\n';
    for (const std::vector<int>& clause : written.clauses) {
        for (int literal : clause) output << literal << ' ';
        output << "0\n";
    }
    for (const Cnf::Parity& parity : written.parities) {
        output << 'x';
        // A negated literal flips the parity a solver requires, so an even
        // constraint is written by negating one of its literals.
        bool flipped = parity.value;
        for (int literal : parity.literals) {
            output << (flipped ? literal : -literal) << ' ';
            flipped = true;
        }
        output << "0\n";
    }
}

Model read_dimacs_model(std::istream& input) {
    Model model;
    std::vector<int> assigned;

    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("s ", 0) == 0) {
            model.answered = true;
            model.satisfiable = line.find("UNSATISFIABLE") == std::string::npos &&
                                line.find("SATISFIABLE") != std::string::npos;
            continue;
        }
        if (line.rfind("v ", 0) != 0) continue;

        std::istringstream values(line.substr(2));
        int literal = 0;
        while (values >> literal) {
            if (literal != 0) assigned.push_back(literal);
        }
    }

    if (!model.satisfiable) return model;
    std::size_t highest = 0;
    for (int literal : assigned) {
        highest = std::max(highest, static_cast<std::size_t>(std::abs(literal)));
    }
    model.values.assign(highest + 1, false);
    for (int literal : assigned) {
        if (literal > 0) model.values[static_cast<std::size_t>(literal)] = true;
    }
    return model;
}

}  // namespace formats
