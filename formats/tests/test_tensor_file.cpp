/// What the tensor reader accepts, and what it refuses before anything reads it.
///
/// The header is three numbers and it is the only place a wrong field can be
/// caught cheaply. Past it a composite characteristic is a tensor like any other,
/// and it took a solver's word to notice, which is the one thing this repository
/// does not want to depend on.
#include <sstream>
#include <stdexcept>
#include <string>

#include "check.h"
#include "tensor_file.h"

namespace {

/// True when `text` is refused as a tensor file.
bool refused(const std::string& text) {
    std::istringstream input(text);
    try {
        linear_algebra::read_tensor(input);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
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
    const linear_algebra::Tensor tensor = linear_algebra::read_tensor(good);
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

    return check::report("tensor file");
}
