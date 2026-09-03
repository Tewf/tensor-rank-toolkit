/// The fixtures parse, and cost the known ranks shown in the reference results
/// before any search runs.
#include <iostream>
#include <string>

#include "check.h"
#include "linear_algebra.h"
#include "tensor_file.h"

namespace {

struct Expectation {
    const char* name;
    int64_t characteristic;
    long long slices;
    long long rows;
    long long columns;
    long long naive_multiplications;
};

constexpr Expectation kExpectations[] = {
    {"f2_5x5", 2, 9, 5, 5, 25},
    {"f2_3x8", 2, 10, 3, 8, 24},
    {"f2_4x7", 2, 10, 4, 7, 28},
    {"f3_3x6", 3, 8, 3, 6, 18},
    // The six that ship for a published target rather than for a measurement
    // made here, from `evidence/fixtures/published-targets.md`, which carries the same
    // numbers. Parsing and the naive count is all this asserts about them: the
    // targets themselves are 13, 15, 13, 19, 25 and 29, and nothing in this
    // repository reaches any of them, so there is no count here to check them
    // against.
    {"gf32_multiplication", 2, 5, 5, 5, 25},
    {"gf64_multiplication", 2, 6, 6, 6, 36},
    {"cyclic_f2_7", 2, 7, 7, 7, 49},
    {"matmul_2x3x4", 2, 8, 6, 12, 24},
    {"matmul_3x3x4", 2, 12, 9, 12, 36},
    {"matmul_3x4x4", 2, 12, 12, 16, 48},
};

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_fixtures <fixtures-directory>\n";
        return 2;
    }
    const std::string directory = argv[1];

    for (const Expectation& expected : kExpectations) {
        const std::string name = expected.name;
        std::cout << name << "\n";

        const formats::Tensor tensor = formats::read_tensor_file(directory + "/" + name + ".tensor");
        check::equal(name + " characteristic", tensor.characteristic, expected.characteristic);
        check::equal(name + " slices", static_cast<long long>(tensor.slices.size()), expected.slices);
        check::equal(name + " rows", static_cast<long long>(tensor.rows()), expected.rows);
        check::equal(name + " columns", static_cast<long long>(tensor.columns()), expected.columns);

        const linear_algebra::ModularField field(tensor.characteristic);
        check::equal(name + " naive multiplications",
                     static_cast<long long>(linear_algebra::multiplication_count(field, tensor.slices)),
                     expected.naive_multiplications);
    }

    return check::report("fixtures");
}
