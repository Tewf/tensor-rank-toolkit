/// DIMACS: that the header counts what follows it, and that a parity means the
/// same thing whichever way it is written.
///
/// A header line disagreeing with the body is the classic way to hand a solver
/// a file it reads differently from how it was meant, and it is invisible until
/// the answer is wrong.
#include <algorithm>
#include <sstream>
#include <string>

#include "check.h"
#include "dimacs_file.h"

namespace {

using linear_algebra::Cnf;

std::size_t body_lines(const std::string& text) {
    std::istringstream lines(text);
    std::string line;
    std::size_t counted = 0;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.rfind("p ", 0) != 0) ++counted;
    }
    return counted;
}

std::string written(const Cnf& formula, bool native_xor) {
    std::ostringstream out;
    linear_algebra::write_dimacs(out, formula, native_xor);
    return out.str();
}

std::size_t header_count(const std::string& text) {
    std::istringstream lines(text);
    std::string ignored;
    std::size_t variables = 0;
    std::size_t clauses = 0;
    lines >> ignored >> ignored >> variables >> clauses;
    return clauses;
}

}  // namespace

int main() {
    Cnf formula;
    const int first = formula.new_variable();
    const int second = formula.new_variable();
    const int third = formula.new_variable();
    formula.add_clause({first, -second});
    formula.add_parity({first, second, third}, true);
    formula.add_parity({first, third}, false);

    const std::string native = written(formula, true);
    check::equal("native header counts the body",
                 static_cast<long long>(header_count(native)),
                 static_cast<long long>(body_lines(native)));
    check::equal("native has one line per parity",
                 static_cast<long long>(std::count(native.begin(), native.end(), 'x')), 2);

    // An odd parity leads with a positive literal, an even one negates the
    // first. Getting this backwards inverts every tensor entry.
    check::equal("odd parity is written positive",
                 native.find("x1 2 3 0") != std::string::npos ? 1 : 0, 1);
    check::equal("even parity negates the first",
                 native.find("x-1 3 0") != std::string::npos ? 1 : 0, 1);

    const std::string plain = written(formula, false);
    check::equal("expanded header counts the body",
                 static_cast<long long>(header_count(plain)),
                 static_cast<long long>(body_lines(plain)));
    check::equal("expanded has no x lines",
                 static_cast<long long>(std::count(plain.begin(), plain.end(), 'x')), 0);
    check::equal("expanding adds variables",
                 linear_algebra::with_parities_expanded(formula).variable_count > 3 ? 1 : 0, 1);

    // A verdict that never arrived is not a verdict.
    std::istringstream silent("c solving\nc gave up\n");
    const auto quiet = linear_algebra::read_dimacs_model(silent);
    check::equal("no verdict is not unsatisfiable", quiet.answered ? 1 : 0, 0);

    // An s line that is not a verdict is not an answer: kissat's alarm handler
    // prints `s UNKNOWN` on the way out, and reading it as one turned a timeout
    // into a refutation (cyclic_f2_7 "refuted" at exactly its cap, 2026-09-01).
    std::istringstream interrupted("s UNKNOWN\nc interrupted\n");
    const auto gave_up = linear_algebra::read_dimacs_model(interrupted);
    check::equal("an s UNKNOWN is not an answer", gave_up.answered ? 1 : 0, 0);

    std::istringstream refused("s UNSATISFIABLE\n");
    const auto no = linear_algebra::read_dimacs_model(refused);
    check::equal("unsatisfiable is answered", no.answered ? 1 : 0, 1);
    check::equal("unsatisfiable is not satisfiable", no.satisfiable ? 1 : 0, 0);

    std::istringstream yes("s SATISFIABLE\nv 1 -2 3 0\n");
    const auto model = linear_algebra::read_dimacs_model(yes);
    check::equal("satisfiable is read", model.satisfiable ? 1 : 0, 1);
    check::equal("variable 1 is true", model.values[1] ? 1 : 0, 1);
    check::equal("variable 2 is false", model.values[2] ? 1 : 0, 0);
    check::equal("variable 3 is true", model.values[3] ? 1 : 0, 1);

    // The generalised expansion: whatever the cutting number and the chaining,
    // the expansion must have exactly one model per original assignment that
    // satisfies the parity (each aux variable is determined), checked by
    // enumerating every assignment of the expanded formula.
    {
        linear_algebra::Cnf parity_formula;
        parity_formula.variable_count = 7;
        parity_formula.parities.push_back({{1, -2, 3, 4, -5, 6, 7}, true});
        const auto models_of = [](const linear_algebra::Cnf& cnf) {
            std::size_t models = 0;
            for (std::size_t assignment = 0; assignment < (std::size_t{1} << cnf.variable_count);
                 ++assignment) {
                const auto holds = [&](int literal) {
                    const bool value = (assignment >> (std::abs(literal) - 1)) & 1;
                    return literal > 0 ? value : !value;
                };
                bool satisfied = true;
                for (const auto& clause : cnf.clauses) {
                    bool any = false;
                    for (int literal : clause) any = any || holds(literal);
                    if (!any) { satisfied = false; break; }
                }
                models += satisfied;
            }
            return models;
        };
        const std::size_t expected = 64;   // half of the 2^7 assignments satisfy an odd parity
        for (std::size_t cut : {std::size_t{3}, std::size_t{4}, std::size_t{6}}) {
            for (bool pooled : {false, true}) {
                const auto expanded =
                    linear_algebra::with_parities_expanded(parity_formula, cut, pooled);
                check::equal("expansion keeps exactly the parity's models", 
                             static_cast<long long>(models_of(expanded)),
                             static_cast<long long>(expected));
            }
        }
        const auto six = linear_algebra::with_parities_expanded(parity_formula, 6, true);
        check::equal("cut 6 slices are length 6",
                     static_cast<long long>(six.clauses.front().size()), 6);
    }

    return check::report("dimacs");
}
