/// The polynomial-time bounds, checked against ranks that are already known.
///
/// A lower bound is only useful if it is sound, and it is only interesting if
/// it is not trivial. Both are asserted here: every bound must be at most the
/// true rank, and on the fixtures it must be strictly greater than one.
#include <string>

#include "check.h"
#include "tensor_file.h"
#include "tensor_flattening.h"

namespace {

using linear_algebra::ModularField;

struct Known {
    const char* name;
    long long rank;
    long long bound;
};

/// Ranks settled elsewhere in this repository, by the exhaustive search and by
/// the solver independently.
constexpr Known kKnown[] = {
    {"f2_2x2", 3, 3},
    {"f2_2x3", 5, 4},
    {"matmul_2x2x2", 7, 4},
    {"w_state", 3, 2},
    {"gf8_multiplication", 6, 3},
};

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";

    for (const Known& known : kKnown) {
        const auto tensor =
            formats::read_tensor_file(fixtures + "/" + std::string(known.name) + ".tensor");
        const ModularField field(tensor.characteristic);

        const std::size_t bound = linear_algebra::flattening_lower_bound(field, tensor.slices);
        const std::string name = known.name;

        // Sound: it is a lower bound, so it can never exceed the true rank.
        check::equal(name + " bound does not exceed the rank",
                     static_cast<long long>(bound) <= known.rank ? 1 : 0, 1);
        check::equal(name + " flattening bound", static_cast<long long>(bound), known.bound);

        // Useful: a sweep starting here skips everything below it. For
        // matmul_2x2x2 that is three solver calls, and for f2_2x3 it is three.
        check::equal(name + " bound beats starting from one",
                     static_cast<long long>(bound) > 1 ? 1 : 0, 1);
    }

    // Every fixture here is concise, so nothing can be compressed away. That is
    // worth pinning: if it were false, the encodings would be larger than they
    // need to be and nothing would say so.
    for (const Known& known : kKnown) {
        const auto tensor =
            formats::read_tensor_file(fixtures + "/" + std::string(known.name) + ".tensor");
        const ModularField field(tensor.characteristic);
        check::equal(std::string(known.name) + " is concise",
                     linear_algebra::is_concise(field, tensor.slices) ? 1 : 0, 1);
    }

    // A tensor that is not concise must be detected. Padding a slice with a
    // zero row makes the first flattening rank deficient.
    {
        const ModularField field(2);
        const auto original = formats::read_tensor_file(fixtures + "/f2_2x2.tensor");
        formats::Tensor padded;
        padded.characteristic = 2;
        for (const auto& slice : original.slices) {
            linear_algebra::ModularMatrix grown(slice.rows() + 1, slice.columns());
            for (std::size_t row = 0; row < slice.rows(); ++row) {
                for (std::size_t column = 0; column < slice.columns(); ++column) {
                    grown(row, column) = slice(row, column);
                }
            }
            padded.slices.push_back(grown);
        }
        check::equal("a padded tensor is not concise",
                     linear_algebra::is_concise(field, padded.slices) ? 1 : 0, 0);
        check::equal("padding does not change the bound",
                     static_cast<long long>(
                         linear_algebra::flattening_lower_bound(field, padded.slices)),
                     3);
    }

    return check::report("tensor flattening");
}
