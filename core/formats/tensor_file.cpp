#include "tensor_file.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

// For `to_string`, which renders a matrix as dense rows. A slice of a tensor
// and a `.matrix` file are the same rows written the same way, and having one
// renderer is what stops the two formats drifting apart entry by entry.
#include "dense_matrix_file.h"

namespace formats {

namespace {

/// Next line with content on it, comments and blank lines skipped.
bool next_meaningful_line(std::istream& input, std::string& line) {
    while (std::getline(input, line)) {
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        return true;
    }
    return false;
}

std::string expect_line(std::istream& input, const char* what) {
    std::string line;
    if (!next_meaningful_line(input, line)) {
        throw std::runtime_error(std::string("tensor file ended before ") + what);
    }
    return line;
}

/// Read `keyword v1 [v2 v3]`, checking the keyword is the one expected.
std::vector<int64_t> read_header_line(std::istream& input, const std::string& keyword,
                                      std::size_t value_count) {
    std::istringstream fields(expect_line(input, keyword.c_str()));
    std::string found;
    fields >> found;
    if (found != keyword) {
        throw std::runtime_error("expected '" + keyword + "' but found '" + found + "'");
    }
    std::vector<int64_t> values(value_count);
    for (std::size_t index = 0; index < value_count; ++index) {
        if (!(fields >> values[index])) {
            throw std::runtime_error("'" + keyword + "' needs " + std::to_string(value_count) +
                                     " values");
        }
    }
    // Anything left on the line is a mistake worth stopping for, not surplus
    // to drop: a header that quietly loses a value reads a different tensor.
    std::string leftover;
    if (fields >> leftover) {
        throw std::runtime_error("'" + keyword + "' takes " + std::to_string(value_count) +
                                 " values and the line carries more, starting at '" + leftover +
                                 "'");
    }
    return values;
}

}  // namespace

Tensor read_tensor(std::istream& input) {
    Tensor tensor;
    tensor.characteristic = read_header_line(input, "field", 1)[0];
    // The message said "must be a prime" while the check only said "at least 2",
    // so `field 4` was read in and left for a backend to notice. Two of the three
    // do notice; the SMT route did not, and a query in a theory of prime fields
    // is not refused for being about a ring.
    if (!linear_algebra::is_prime(tensor.characteristic)) {
        throw std::runtime_error("field " + std::to_string(tensor.characteristic) +
                                 ": the characteristic must be a prime. GF(2^k) is written as a "
                                 "bigger tensor over GF(2), which is what the gf4, gf8 and gf16 "
                                 "fixtures are");
    }

    const std::vector<int64_t> shape = read_header_line(input, "shape", 3);
    const auto slice_count = static_cast<std::size_t>(shape[0]);
    const auto rows = static_cast<std::size_t>(shape[1]);
    const auto columns = static_cast<std::size_t>(shape[2]);

    tensor.slices.reserve(slice_count);
    for (std::size_t index = 0; index < slice_count; ++index) {
        linear_algebra::ModularMatrix slice(rows, columns);
        for (std::size_t row = 0; row < rows; ++row) {
            std::istringstream entries(expect_line(input, "a matrix row"));
            for (std::size_t column = 0; column < columns; ++column) {
                if (!(entries >> slice(row, column))) {
                    throw std::runtime_error("slice " + std::to_string(index) + " row " +
                                             std::to_string(row) + " is short of entries");
                }
            }
            // A row carrying more entries than `shape` declares was silently
            // truncated here once: the answer came back verified and wrong
            // for the file the user thought they wrote. Refuse it instead.
            std::string leftover;
            if (entries >> leftover) {
                throw std::runtime_error("slice " + std::to_string(index) + " row " +
                                         std::to_string(row) +
                                         " has more entries than 'shape' declares");
            }
        }
        tensor.slices.push_back(std::move(slice));
    }
    return tensor;
}

Tensor read_tensor_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open tensor file: " + path);
    return read_tensor(input);
}

void write_tensor(std::ostream& output, const Tensor& tensor, const std::string& comment) {
    std::istringstream lines(comment);
    for (std::string line; std::getline(lines, line);) output << "# " << line << "\n";
    output << "field " << tensor.characteristic << "\n";
    output << "shape " << tensor.slices.size() << " " << tensor.rows() << " " << tensor.columns()
           << "\n";
    // A blank line before every slice, the first one included. It is what the
    // fixtures carry, and the reader skips blank lines anyway, so this is for
    // whoever opens the file rather than for anything that parses it.
    for (const linear_algebra::ModularMatrix& slice : tensor.slices) {
        output << "\n" << to_string(slice);
    }
}

}  // namespace formats
