/// Write out a bilinear map as a tensor file.
///
/// Outputs tensors as files, enabling reproducible and scriptable tests of
/// tensor decomposition algorithms.
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "exit_code.h"
#include "map_construction.h"
#include "tensor_file.h"

namespace {

void usage() {
    std::cerr << "usage: make-tensor --polynomial <p> <left-terms> <right-terms>\n"
                 "       make-tensor --matmul <p> <n> <m> <k>\n"
                 "       make-tensor --cyclic <p> <length>\n"
                 "       make-tensor --field <p> <modulus coefficients, highest degree first>\n"
                 "\n"
                 "  --polynomial 2 5 5     multiplying two 5-term polynomials over GF(2)\n"
                 "  --matmul 2 2 2 2       <2,2,2>: 2x2 by 2x2 matrices, where Strassen starts\n"
                 "  --cyclic 2 5           multiplying modulo x^5 - 1 over GF(2)\n"
                 "  --field 2 1 1 1        multiplying in GF(2^2), modulus x^2 + x + 1\n"
                 "\n"
                 "Writes a tensor file on standard output.\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string mode = argv[1];
    const int64_t characteristic = std::stoll(argv[2]);
    const bilinear_rank::Field field(characteristic);

    std::vector<bilinear_rank::Matrix> slices;
    std::string description;

    try {
        if (mode == "--polynomial" && argc == 5) {
            const auto left = static_cast<std::size_t>(std::stoull(argv[3]));
            const auto right = static_cast<std::size_t>(std::stoull(argv[4]));
            slices = bilinear_rank::polynomial_multiplication_tensor(left, right);
            description = "Polynomial multiplication of " + std::string(argv[3]) +
                          " coefficients by " + argv[4] + ", over GF(" + argv[2] + ").";
        } else if (mode == "--matmul" && argc == 6) {
            const auto rows = static_cast<std::size_t>(std::stoull(argv[3]));
            const auto inner = static_cast<std::size_t>(std::stoull(argv[4]));
            const auto columns = static_cast<std::size_t>(std::stoull(argv[5]));
            slices = bilinear_rank::matrix_multiplication_tensor(rows, inner, columns);
            description = "Matrix multiplication <" + std::string(argv[3]) + "," + argv[4] + "," +
                          argv[5] + ">, over GF(" + argv[2] + ").";
        } else if (mode == "--cyclic" && argc == 4) {
            const auto length = static_cast<std::size_t>(std::stoull(argv[3]));
            slices = bilinear_rank::cyclic_convolution_tensor(length);
            description = "Cyclic convolution of length " + std::string(argv[3]) + ", that is "
                          "multiplication modulo x^" + argv[3] + " - 1, over GF(" + argv[2] + ").";
        } else if (mode == "--field" && argc > 4) {
            bilinear_rank::Polynomial modulus;
            for (int argument = 3; argument < argc; ++argument) {
                modulus.push_back(std::stoll(argv[argument]));
            }
            slices = bilinear_rank::field_multiplication_tensor(field, modulus);
            description = "Multiplication in GF(" + std::string(argv[2]) + "^" +
                          std::to_string(modulus.size() - 1) + "), modulus given highest degree" +
                          " first.";
        } else {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
    } catch (const std::exception& problem) {
        // Was 1. This command asks no question, so it has no "no" to give: a
        // construction that threw is a tool that could not run.
        std::cerr << "make-tensor: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }

    if (slices.empty()) {
        std::cerr << "make-tensor: that produced no slices\n";
        // Was 1, and for the same reason: an empty tensor is not an answer
        // about anything, it is a construction that did not happen.
        return cli::exit_status(cli::ExitCode::Error);
    }

    linear_algebra::Tensor tensor;
    tensor.characteristic = characteristic;
    tensor.slices = std::move(slices);

    // Two lines, because the naive cost is what any rank claim about this file
    // is an improvement on, and a fixture that has been copied about should
    // still carry the number it is measured against.
    const std::string comment =
        description + "\nNaive cost " +
        std::to_string(linear_algebra::multiplication_count(field, tensor.slices)) +
        " multiplications, written by make-tensor.";
    linear_algebra::write_tensor(std::cout, tensor, comment);
    return cli::exit_status(cli::ExitCode::Yes);
}
