/// What the dense rational reader refuses: a malformed row stops the run,
/// it never shrinks into a smaller matrix that happens to parse.
///
/// The tensor reader once dropped a row's surplus entries silently, and a
/// mistyped file came back as a verified answer to a different question.
/// This reader shared the hole; both directions of malformation must refuse.
#include <sstream>
#include <stdexcept>
#include <string>

#include "check.h"
#include "dense_matrix_file.h"

namespace {

/// True when `text` is refused as a dense matrix file.
bool refused(const std::string& text) {
    std::istringstream input(text);
    try {
        formats::read_rational_matrix(input);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

}  // namespace

int main() {
    check::equal("a well-formed matrix is read",
                 refused("shape 2 2\n1 0\n4/9 -2/3\n") ? 1 : 0, 0);
    check::equal("a row short of entries is refused",
                 refused("shape 2 2\n1\n0 1\n") ? 1 : 0, 1);
    check::equal("a row with extra entries is refused",
                 refused("shape 1 2\n1 0 1\n") ? 1 : 0, 1);
    check::equal("a zero denominator is refused",
                 refused("shape 1 1\n1/0\n") ? 1 : 0, 1);
    check::equal("a non-number is refused",
                 refused("shape 1 1\nx\n") ? 1 : 0, 1);
    return check::report("dense matrix file");
}
