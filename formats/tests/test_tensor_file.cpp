/// What the tensor reader accepts, what it refuses before anything reads it,
/// and that the writer speaks the same format back.
///
/// The header is three numbers and it is the only place a wrong field can be
/// caught cheaply. Past it a composite characteristic is a tensor like any other,
/// and it took a solver's word to notice, which is the one thing this repository
/// does not want to depend on.
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "check.h"
#include "tensor_file.h"

namespace {

/// True when `text` is refused as a tensor file.
bool refused(const std::string& text) {
    std::istringstream input(text);
    try {
        formats::read_tensor(input);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

/// A tensor with every residue of GF(5) somewhere in it, and no symmetry
/// between its axes.
///
/// Three slices of two rows by three columns, all four numbers different, so a
/// writer that transposed a slice or swapped the shape fields would produce
/// something the reader either refuses or reads back changed. A cube of zeroes
/// and ones would survive both mistakes.
formats::Tensor sample_tensor() {
    formats::Tensor tensor;
    tensor.characteristic = 5;
    for (std::size_t index = 0; index < 3; ++index) {
        linear_algebra::ModularMatrix slice(2, 3);
        for (std::size_t row = 0; row < slice.rows(); ++row) {
            for (std::size_t column = 0; column < slice.columns(); ++column) {
                slice(row, column) = static_cast<int64_t>((index + row + 2 * column) % 5);
            }
        }
        tensor.slices.push_back(std::move(slice));
    }
    return tensor;
}

/// How many entries the two tensors disagree on, shapes included.
long long entry_differences(const formats::Tensor& left,
                            const formats::Tensor& right) {
    if (left.slices.size() != right.slices.size() || left.rows() != right.rows() ||
        left.columns() != right.columns()) {
        return -1;
    }
    long long differences = 0;
    for (std::size_t index = 0; index < left.slices.size(); ++index) {
        for (std::size_t row = 0; row < left.rows(); ++row) {
            for (std::size_t column = 0; column < left.columns(); ++column) {
                if (left.slices[index](row, column) != right.slices[index](row, column)) {
                    ++differences;
                }
            }
        }
    }
    return differences;
}

}  // namespace

int main() {
    std::istringstream good(
        "# a comment, and a blank line\n"
        "\n"
        "field 3\n"
        "shape 2 1 2\n"
        "1 0\n"
        "0 2\n");
    const formats::Tensor tensor = formats::read_tensor(good);
    check::equal("the characteristic is read", tensor.characteristic, 3);
    check::equal("both slices are read", static_cast<long long>(tensor.slices.size()), 2);
    check::equal("and the entries with them", static_cast<long long>(tensor.slices[1](0, 1)), 2);

    // The characteristic must be a prime, not merely at least two. Modulo a
    // composite there are zero divisors, so rank is not the quantity anything
    // downstream measures: both CNF encoders refuse such a tensor and cvc5's
    // theory of finite fields is a decision procedure for prime fields.
    check::equal("field 4 is refused", refused("field 4\nshape 1 1 1\n1\n") ? 1 : 0, 1);
    check::equal("field 9 is refused too", refused("field 9\nshape 1 1 1\n1\n") ? 1 : 0, 1);
    check::equal("field 1 is refused", refused("field 1\nshape 1 1 1\n1\n") ? 1 : 0, 1);
    check::equal("field 2 is accepted", refused("field 2\nshape 1 1 1\n1\n") ? 1 : 0, 0);

    check::equal("a missing keyword is refused", refused("characteristic 2\n") ? 1 : 0, 1);
    check::equal("a slice short of entries is refused",
                 refused("field 2\nshape 1 2 2\n1 0\n1\n") ? 1 : 0, 1);

    // The round trip. Every check above compares the reader against a string
    // typed here, which says nothing about what the repository itself writes;
    // this is the only one where both halves of the format have to agree.
    const formats::Tensor written = sample_tensor();
    std::ostringstream text;
    formats::write_tensor(text, written, "a round trip\nover two comment lines");
    std::istringstream reread(text.str());
    const formats::Tensor read_back = formats::read_tensor(reread);

    check::equal("the characteristic comes back", read_back.characteristic,
                 written.characteristic);
    check::equal("the slices come back", static_cast<long long>(read_back.slices.size()),
                 static_cast<long long>(written.slices.size()));
    check::equal("the rows come back", static_cast<long long>(read_back.rows()),
                 static_cast<long long>(written.rows()));
    check::equal("the columns come back", static_cast<long long>(read_back.columns()),
                 static_cast<long long>(written.columns()));
    check::equal("no entry changed on the way", entry_differences(written, read_back), 0);

    // The writer's own comment block has to be something the reader skips, or
    // the format would only round trip when nothing described it.
    check::equal("the comment is written as two `#` lines",
                 static_cast<long long>(text.str().find("# a round trip\n"
                                                        "# over two comment lines\n")),
                 0);

    return check::report("tensor file");
}
