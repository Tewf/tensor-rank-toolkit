#include "smtlib_file.h"

#include <sstream>

namespace linear_algebra {

std::string SmtProblem::literal(std::size_t value) {
    return "(as ff" + std::to_string(value) + " F)";
}

void write_smtlib(std::ostream& output, const SmtProblem& problem) {
    output << "(set-logic QF_FF)\n";
    output << "(set-option :produce-models true)\n";
    output << "(define-sort F () (_ FiniteField " << problem.characteristic << "))\n";
    for (const std::string& name : problem.constants) {
        output << "(declare-const " << name << " F)\n";
    }
    for (const std::string& assertion : problem.assertions) {
        output << "(assert " << assertion << ")\n";
    }
    output << "(check-sat)\n(get-model)\n";
}

namespace {

/// Read `#f<value>m<modulus>` starting at `at`, or leave `value` untouched.
bool read_field_value(const std::string& text, std::size_t at, std::size_t& value) {
    if (at + 1 >= text.size() || text[at] != '#' || text[at + 1] != 'f') return false;

    std::size_t cursor = at + 2;
    std::size_t parsed = 0;
    bool any = false;
    while (cursor < text.size() && text[cursor] >= '0' && text[cursor] <= '9') {
        parsed = parsed * 10 + static_cast<std::size_t>(text[cursor] - '0');
        ++cursor;
        any = true;
    }
    if (!any || cursor >= text.size() || text[cursor] != 'm') return false;

    value = parsed;
    return true;
}

}  // namespace

SmtModel read_smtlib_model(std::istream& input) {
    SmtModel model;

    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("unsat", 0) == 0) {
            model.answered = true;
            model.satisfiable = false;
            continue;
        }
        if (line.rfind("sat", 0) == 0) {
            model.answered = true;
            model.satisfiable = true;
            continue;
        }

        // (define-fun a_0_0 () (_ FiniteField 3) #f2m3)
        const std::size_t opens = line.find("(define-fun ");
        if (opens == std::string::npos) continue;

        std::istringstream fields(line.substr(opens + 12));
        std::string name;
        fields >> name;
        if (name.empty()) continue;

        const std::size_t marker = line.find("#f", opens);
        std::size_t value = 0;
        if (marker != std::string::npos && read_field_value(line, marker, value)) {
            model.values[name] = value;
        }
    }
    return model;
}

}  // namespace linear_algebra
