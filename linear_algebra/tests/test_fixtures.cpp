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

        const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(directory + "/" + name + ".tensor");
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
