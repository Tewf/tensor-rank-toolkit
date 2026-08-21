#include "sms_file.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace linear_algebra {

namespace {

/// One word of the file, and whether it was the last one on its line.
///
/// The flag is carried because SMS is line-sensitive in exactly one place, and
/// it is the place a token stream would otherwise paper over. See `tokens_of`.
struct Token {
    std::string text;
    bool ends_line = false;
};

/// Every token of the file, with `#` comments and blank lines removed.
///
/// Read whole rather than streamed because the header is not the first line in
/// practice: every matrix shipped with PLinOpt opens with a `#` describing the
/// algorithm it encodes, and `input >> rows` on such a file fails on the hash.
std::vector<Token> tokens_of(std::istream& input) {
    std::vector<Token> tokens;
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        std::istringstream fields(line);
        std::size_t first = tokens.size();
        for (std::string token; fields >> token;) tokens.push_back({token, false});
        if (tokens.size() > first) tokens.back().ends_line = true;
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

/// An entry is a rational here, and SMS does not promise that.
///
/// LinBox reads entries over whatever ring the stream was instantiated with, so
/// the same file can hold something else: `sms2pretty` opens a
/// `MatrixStream<QPol>` (`src/sms2pretty.cpp:95`) and PLinOpt ships
/// `data/2x2x2_7_DPS-accurate-X_{L,R,P}.sms`, whose entries are `2X`, `3*X` and
/// `(1/6)*X` in an indeterminate its checkers are told about with `-P "X^2-3"`.
/// Three of its 153 matrices are that family. Nothing here computes over `Q[X]`,
/// so the refusal says which file it is looking at rather than reporting a digit
/// it did not find.
void require_no_indeterminate(const std::string& text) {
    if (text.find_first_of("XxYy*^") == std::string::npos) return;
    throw std::runtime_error("SMS value '" + text +
                             "' is a polynomial in an indeterminate, not a rational. Entries "
                             "here live in Q or in GF(p); PLinOpt's -P family does not");
}

Givaro::Rational parse_value(const std::string& text) {
    require_no_indeterminate(text);
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

/// A triple's value has to be the last word on its line, and this is the one
/// rule a token stream would silently break.
///
/// Two triples on one line is not a layout choice the format leaves open. LinBox
/// reads the value with Givaro's `operator>>(std::istream&, Rational&)`
/// (`/usr/include/givaro/givrational.h:208`), which takes the rest of the line
/// and strips its spaces, so `1 1 5 3 2 7` is one entry of 5327 and not two
/// entries. Checked against `plinopt/bin/sms2pretty`, which prints exactly that
/// and reports one nonzero, with no warning of any kind.
///
/// **So a permissive reader here is not generosity, it is disagreement.** We
/// would hold a different matrix from the one the reference tools hold, from the
/// same bytes, with nothing on either side saying so. Refusing turns a silent
/// cross-tool disagreement into a loud local one. Splitting a triple *across*
/// lines stays legal, because LinBox accepts that: only the value is greedy, and
/// only to the end of its own line.
void require_value_ends_its_line(const std::vector<Token>& tokens, std::size_t value) {
    if (tokens[value].ends_line) return;
    throw std::runtime_error("SMS value '" + tokens[value].text +
                             "' is not the last word on its line, and LinBox would read it "
                             "together with what follows as a single number");
}

}  // namespace

RationalMatrix read_sms(std::istream& input) {
    const std::vector<Token> tokens = tokens_of(input);
    if (tokens.size() < 3) throw std::runtime_error("SMS needs a header: rows columns type");
    if (!is_type_letter(tokens[2].text)) {
        throw std::runtime_error("SMS type must be one of M m I i R r P p, not '" +
                                 tokens[2].text + "'");
    }

    const std::size_t rows = parse_count(tokens[0].text);
    const std::size_t columns = parse_count(tokens[1].text);
    RationalMatrix matrix(rows, columns);

    bool terminated = false;
    for (std::size_t at = 3; at + 2 < tokens.size(); at += 3) {
        const Givaro::Integer row = parse_integer(tokens[at].text);
        const Givaro::Integer column = parse_integer(tokens[at + 1].text);
        if (row == 0 && column == 0) {  // the terminator; its value is ignored
            terminated = true;
            break;
        }
        if (row < 1 || column < 1 || row > Givaro::Integer(static_cast<int64_t>(rows)) ||
            column > Givaro::Integer(static_cast<int64_t>(columns))) {
            throw std::runtime_error("SMS entry " + tokens[at].text + " " +
                                     tokens[at + 1].text + " is outside the declared shape");
        }
        require_value_ends_its_line(tokens, at + 2);
        // SMS counts from one. Entries are not required to arrive in order:
        // hpac.imag.fr asks for lexicographic triples and PLinOpt's own data
        // does not obey, so nothing here depends on it.
        matrix(static_cast<std::size_t>(Givaro::Integer(row)) - 1,
               static_cast<std::size_t>(Givaro::Integer(column)) - 1) =
            parse_value(tokens[at + 2].text);
    }
    if (!terminated) throw std::runtime_error("SMS ended before its 0 0 0 terminator");
    return matrix;
}

ModularMatrix read_sms(std::istream& input, const ModularField& field) {
    const RationalMatrix rationals = read_sms(input);
    ModularMatrix matrix(rationals.rows(), rationals.columns());
    for (std::size_t row = 0; row < rationals.rows(); ++row) {
        for (std::size_t column = 0; column < rationals.columns(); ++column) {
            const Givaro::Rational& value = rationals(row, column);
            ModularField::Element numerator, denominator;
            field.init(numerator, value.nume());
            field.init(denominator, value.deno());
            if (field.isZero(denominator)) {
                std::ostringstream refusal;
                refusal << "SMS entry " << (row + 1) << " " << (column + 1) << " is "
                        << value.nume() << "/" << value.deno()
                        << ", whose denominator vanishes modulo " << field.characteristic();
                throw std::runtime_error(refusal.str());
            }
            field.div(matrix(row, column), numerator, denominator);
        }
    }
    return matrix;
}

ModularMatrix read_sms_file(const std::string& path, const ModularField& field) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open SMS file: " + path);
    return read_sms(input, field);
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
