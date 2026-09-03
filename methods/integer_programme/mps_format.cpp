#include "mps_format.h"

#include <sstream>
#include <vector>

#include "whole_numbers.h"

namespace integer_programme {

namespace {

std::string text_of(const Givaro::Integer& value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

Givaro::Integer common_divisor(Givaro::Integer left, Givaro::Integer right) {
    while (right != 0) {
        const Givaro::Integer remainder = left % right;
        left = right;
        right = remainder;
    }
    return left < 0 ? -left : left;
}

/// What every entry of a row must be multiplied by to become whole.
Givaro::Integer common_denominator(const std::vector<Number>& values) {
    Givaro::Integer multiplier = 1;
    for (const Number& value : values) {
        multiplier = multiplier / common_divisor(multiplier, value.deno()) * value.deno();
    }
    return multiplier;
}

std::string decimal_of(const Number& value) {
    if (value.deno() == 1) return text_of(value.nume());

    Givaro::Integer scale = 1;
    for (int digit = 0; digit < 9; ++digit) scale *= 10;
    const bool negative = value.nume() < 0;
    const Givaro::Integer magnitude = negative ? -value.nume() : value.nume();
    const Givaro::Integer scaled = magnitude * scale / value.deno();

    std::string fraction = text_of(scaled % scale);
    fraction.insert(0, 9 - fraction.size(), '0');
    while (!fraction.empty() && fraction.back() == '0') fraction.pop_back();

    std::string text = (negative ? "-" : "") + text_of(scaled / scale);
    if (!fraction.empty()) text += "." + fraction;
    return text;
}

/// A line in the fixed layout: fields at columns 2, 5, 15, 25, 40 and 50. Only
/// ever one value per line, so field four runs to the end and can be as long as
/// the arithmetic made it.
std::string line_of(const std::string& first, const std::string& second,
                    const std::string& third = "", const std::string& fourth = "",
                    const std::string& fifth = "") {
    std::string line = " " + first;
    line.resize(4, ' ');
    line += second;
    line.resize(14, ' ');
    line += third;
    if (!fourth.empty()) {
        line.resize(24, ' ');
        line += fourth;
    }
    if (!fifth.empty()) {
        line.resize(39, ' ');
        line += fifth;
    }
    return line;
}

std::string row_name(std::size_t index) { return "c" + std::to_string(index + 1); }

char relation_letter(Relation relation) {
    switch (relation) {
        case Relation::LessOrEqual: return 'L';
        case Relation::GreaterOrEqual: return 'G';
        case Relation::Equal: return 'E';
    }
    return 'E';
}

/// A bound on a whole variable can be moved to the nearest integer inside it
/// without changing which points satisfy it, which keeps it exact on the way out.
std::string bound_text(const Variable& variable, bool upper) {
    if (!variable.integral) return decimal_of(upper ? variable.upper : variable.lower);
    return text_of(upper ? floor_of(variable.upper) : ceiling_of(variable.lower));
}

}  // namespace

std::string column_name(std::size_t index) { return "x" + std::to_string(index + 1); }

std::string mps_of(const IntegerProgramme& programme) {
    const std::size_t width = programme.variables.size();

    // MPS has no direction: the objective row is always minimised, and the
    // `OBJSENSE` section that would say otherwise is a later extension not every
    // reader here has. Maximising `c` is minimising `−c`, and the negation never
    // leaves this file because the objective the caller is told is recomputed
    // from the model. Without this every maximisation silently comes back at its
    // minimum, which is feasible, which is why only a battery catches it.
    std::vector<Number> objective = programme.objective;
    objective.resize(width, Number(0));
    if (programme.sense == Sense::Maximise) {
        for (Number& entry : objective) entry = Number(0) - entry;
    }
    const Givaro::Integer objective_scale = common_denominator(objective);

    std::vector<Givaro::Integer> scale(programme.constraints.size(), 1);
    // Sparse rows are stored by row, and MPS is written by column, so the one
    // transposition happens here rather than in a scan per column.
    std::vector<std::vector<std::pair<std::size_t, Number>>> by_column(width);
    for (std::size_t row = 0; row < programme.constraints.size(); ++row) {
        std::vector<Number> entries;
        for (const Coefficient& term : programme.constraints[row].terms) {
            entries.push_back(term.value);
        }
        entries.push_back(programme.constraints[row].bound);
        scale[row] = common_denominator(entries);
        for (const Coefficient& term : programme.constraints[row].terms) {
            if (term.variable < width) by_column[term.variable].push_back({row, term.value});
        }
    }

    std::ostringstream out;
    // Section headers start at column one; only the data lines are indented.
    out << "NAME          PROGRAMME\nROWS\n";
    out << line_of("N", "obj") << "\n";
    for (std::size_t row = 0; row < programme.constraints.size(); ++row) {
        out << line_of(std::string(1, relation_letter(programme.constraints[row].relation)),
                       row_name(row))
            << "\n";
    }

    out << "COLUMNS\n";
    bool among_integers = false;
    std::size_t marker = 0;
    for (std::size_t index = 0; index < width; ++index) {
        if (programme.variables[index].integral != among_integers) {
            among_integers = programme.variables[index].integral;
            out << line_of("", "M" + std::to_string(++marker), "'MARKER'", "",
                           among_integers ? "'INTORG'" : "'INTEND'")
                << "\n";
        }
        const Number scaled = objective[index] * Number(objective_scale);
        out << line_of("", column_name(index), "obj", text_of(scaled.nume())) << "\n";
        for (const auto& [row, coefficient] : by_column[index]) {
            const Number entry = coefficient * Number(scale[row]);
            out << line_of("", column_name(index), row_name(row), text_of(entry.nume())) << "\n";
        }
    }
    if (among_integers) {
        out << line_of("", "M" + std::to_string(++marker), "'MARKER'", "", "'INTEND'") << "\n";
    }

    out << "RHS\n";
    for (std::size_t row = 0; row < programme.constraints.size(); ++row) {
        const Number bound = programme.constraints[row].bound * Number(scale[row]);
        if (bound == Number(0)) continue;
        out << line_of("", "rhs", row_name(row), text_of(bound.nume())) << "\n";
    }

    out << "BOUNDS\n";
    for (std::size_t index = 0; index < width; ++index) {
        const Variable& variable = programme.variables[index];
        const std::string name = column_name(index);
        if (variable.bounded_below) {
            out << line_of("LO", "BND", name, bound_text(variable, false)) << "\n";
        } else {
            out << line_of("MI", "BND", name) << "\n";
        }
        if (variable.bounded_above) {
            out << line_of("UP", "BND", name, bound_text(variable, true)) << "\n";
        } else {
            out << line_of("PL", "BND", name) << "\n";
        }
    }

    out << "ENDATA\n";
    return out.str();
}

}  // namespace integer_programme
