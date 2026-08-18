#include "dense_matrix_file.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace linear_algebra {

namespace {

bool next_meaningful_line(std::istream& input, std::string& line) {
    while (std::getline(input, line)) {
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        return true;
    }
    return false;
}

/// Givaro's Integer takes a string without telling you when it could not read
/// it, so the shape is checked here rather than discovered as a wrong answer.
Givaro::Integer parse_integer(const std::string& text) {
    if (text.empty()) throw std::runtime_error("empty number");
    const std::size_t digits_from = (text[0] == '-' || text[0] == '+') ? 1 : 0;
    if (digits_from == text.size()) throw std::runtime_error("'" + text + "' has no digits");
    for (std::size_t index = digits_from; index < text.size(); ++index) {
        if (text[index] < '0' || text[index] > '9') {
            throw std::runtime_error("'" + text + "' is not an integer");
        }
    }
    return Givaro::Integer(text.c_str());
}

/// `4/9`, `-2/3`, `0`, `7`. Anything else is a mistake worth stopping for.
Givaro::Rational parse_rational(const std::string& text) {
    const std::size_t slash = text.find('/');
    if (slash == std::string::npos) {
        return Givaro::Rational(parse_integer(text), Givaro::Integer(1));
    }
    const Givaro::Integer denominator = parse_integer(text.substr(slash + 1));
    if (denominator == 0) throw std::runtime_error("zero denominator in '" + text + "'");
    return Givaro::Rational(parse_integer(text.substr(0, slash)), denominator);
}

}  // namespace

RationalMatrix read_rational_matrix(std::istream& input) {
    std::string line;
    if (!next_meaningful_line(input, line)) throw std::runtime_error("matrix file is empty");

    std::istringstream header(line);
    std::string keyword;
    std::size_t rows = 0;
    std::size_t columns = 0;
    if (!(header >> keyword >> rows >> columns) || keyword != "shape") {
        throw std::runtime_error("expected 'shape <rows> <columns>', found: " + line);
    }

    RationalMatrix matrix(rows, columns);
    for (std::size_t row = 0; row < rows; ++row) {
        if (!next_meaningful_line(input, line)) {
            throw std::runtime_error("matrix file ended at row " + std::to_string(row));
        }
        std::istringstream entries(line);
        std::string text;
        for (std::size_t column = 0; column < columns; ++column) {
            if (!(entries >> text)) {
                throw std::runtime_error("row " + std::to_string(row) + " is short of entries");
            }
            matrix(row, column) = parse_rational(text);
        }
    }
    return matrix;
}

RationalMatrix read_rational_matrix_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open matrix file: " + path);
    return read_rational_matrix(input);
}

std::string to_string(const ModularMatrix& matrix) {
    std::ostringstream text;
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            text << (column == 0 ? "" : " ") << matrix(row, column);
        }
        text << "\n";
    }
    return text.str();
}

void write_matrix_file(const std::string& path, const std::string& comment,
                       const ModularMatrix& matrix) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot write matrix file: " + path);
    std::istringstream lines(comment);
    std::string line;
    while (std::getline(lines, line)) output << "# " << line << "\n";
    output << "shape " << matrix.rows() << " " << matrix.columns() << "\n";
    output << to_string(matrix);
}

std::string to_string(const RationalMatrix& matrix) {
    std::ostringstream text;
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            const Givaro::Rational& entry = matrix(row, column);
            text << (column == 0 ? "" : " ");
            if (entry.deno() == 1) {
                text << entry.nume();
            } else {
                text << entry.nume() << "/" << entry.deno();
            }
        }
        text << "\n";
    }
    return text.str();
}

}  // namespace linear_algebra
