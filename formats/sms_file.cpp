#include "sms_file.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace linear_algebra {

namespace {

/// Every token of the file, with `#` comments and blank lines removed.
///
/// Read whole rather than streamed because the header is not the first line in
/// practice: every matrix shipped with PLinOpt opens with a `#` describing the
/// algorithm it encodes, and `input >> rows` on such a file fails on the hash.
/// Collecting tokens also makes the reader indifferent to how many triples a
/// writer puts on a line, which the format does not fix.
std::vector<std::string> tokens_of(std::istream& input) {
    std::vector<std::string> tokens;
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        std::istringstream fields(line);
        for (std::string token; fields >> token;) tokens.push_back(token);
    }
    return tokens;
}

Givaro::Integer parse_integer(const std::string& text) {
    if (text.empty()) throw std::runtime_error("empty number in SMS");
    const std::size_t digits_from = (text[0] == '-' || text[0] == '+') ? 1 : 0;
    if (digits_from == text.size()) throw std::runtime_error("'" + text + "' has no digits");
    for (std::size_t index = digits_from; index < text.size(); ++index) {
        if (text[index] < '0' || text[index] > '9') {
            throw std::runtime_error("'" + text + "' is not an integer");
        }
    }
    return Givaro::Integer(text.c_str());
}

Givaro::Rational parse_value(const std::string& text) {
    const std::size_t slash = text.find('/');
    if (slash == std::string::npos) {
        return Givaro::Rational(parse_integer(text), Givaro::Integer(1));
    }
    const Givaro::Integer denominator = parse_integer(text.substr(slash + 1));
    if (denominator == 0) throw std::runtime_error("zero denominator in '" + text + "'");
    return Givaro::Rational(parse_integer(text.substr(0, slash)), denominator);
}

std::size_t parse_count(const std::string& text) {
    const Givaro::Integer count = parse_integer(text);
    if (count < 0) throw std::runtime_error("negative dimension '" + text + "' in SMS header");
    return static_cast<std::size_t>(Givaro::Integer(count));
}

/// LinBox reads any of these, and files in the wild use most of them.
bool is_type_letter(const std::string& type) {
    return type.size() == 1 && std::string("MmIiRrPp").find(type[0]) != std::string::npos;
}

}  // namespace

RationalMatrix read_sms(std::istream& input) {
    const std::vector<std::string> tokens = tokens_of(input);
    if (tokens.size() < 3) throw std::runtime_error("SMS needs a header: rows columns type");
    if (!is_type_letter(tokens[2])) {
        throw std::runtime_error("SMS type must be one of M m I i R r P p, not '" + tokens[2] +
                                 "'");
    }

    const std::size_t rows = parse_count(tokens[0]);
    const std::size_t columns = parse_count(tokens[1]);
    RationalMatrix matrix(rows, columns);

    bool terminated = false;
    for (std::size_t at = 3; at + 2 < tokens.size(); at += 3) {
        const Givaro::Integer row = parse_integer(tokens[at]);
        const Givaro::Integer column = parse_integer(tokens[at + 1]);
        if (row == 0 && column == 0) {  // the terminator; its value is ignored
            terminated = true;
            break;
        }
        if (row < 1 || column < 1 || row > Givaro::Integer(static_cast<int64_t>(rows)) ||
            column > Givaro::Integer(static_cast<int64_t>(columns))) {
            throw std::runtime_error("SMS entry " + tokens[at] + " " + tokens[at + 1] +
                                     " is outside the declared shape");
        }
        // SMS counts from one. Entries are not required to arrive in order:
        // hpac.imag.fr asks for lexicographic triples and PLinOpt's own data
        // does not obey, so nothing here depends on it.
        matrix(static_cast<std::size_t>(Givaro::Integer(row)) - 1,
               static_cast<std::size_t>(Givaro::Integer(column)) - 1) =
            parse_value(tokens[at + 2]);
    }
    if (!terminated) throw std::runtime_error("SMS ended before its 0 0 0 terminator");
    return matrix;
}

RationalMatrix read_sms_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open SMS file: " + path);
    return read_sms(input);
}

void write_sms(std::ostream& output, const RationalMatrix& matrix) {
    const RationalField field;
    output << matrix.rows() << " " << matrix.columns() << " R\n";
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            const Givaro::Rational& value = matrix(row, column);
            if (field.isZero(value)) continue;
            output << (row + 1) << " " << (column + 1) << " " << value.nume();
            if (value.deno() != 1) output << "/" << value.deno();
            output << "\n";
        }
    }
    output << "0 0 0\n";
}

void write_sms_file(const std::string& path, const std::string& comment,
                    const ModularMatrix& matrix) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot write SMS file: " + path);
    std::istringstream lines(comment);
    for (std::string line; std::getline(lines, line);) output << "# " << line << "\n";
    write_sms(output, matrix);
}

void write_sms(std::ostream& output, const ModularMatrix& matrix) {
    output << matrix.rows() << " " << matrix.columns() << " M\n";
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            const int64_t value = matrix(row, column);
            if (value == 0) continue;
            output << (row + 1) << " " << (column + 1) << " " << value << "\n";
        }
    }
    output << "0 0 0\n";
}

}  // namespace linear_algebra
