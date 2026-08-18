#include "tensor_file.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace linear_algebra {

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
    if (!is_prime(tensor.characteristic)) {
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
        ModularMatrix slice(rows, columns);
        for (std::size_t row = 0; row < rows; ++row) {
            std::istringstream entries(expect_line(input, "a matrix row"));
            for (std::size_t column = 0; column < columns; ++column) {
                if (!(entries >> slice(row, column))) {
                    throw std::runtime_error("slice " + std::to_string(index) + " row " +
                                             std::to_string(row) + " is short of entries");
                }
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

}  // namespace linear_algebra
