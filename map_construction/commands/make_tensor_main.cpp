/// Write out a bilinear map as a tensor file.
///
/// Outputs tensors as files, enabling reproducible and scriptable tests of
/// tensor decomposition algorithms.
#include <string>
#include <utility>
#include <vector>

#include "arguments.h"
#include "exit_code.h"
#include "map_construction.h"
#include "report.h"
#include "tensor_file.h"

namespace {

void usage() {
    cli::note() << "usage: make-tensor --polynomial <p> <left-terms> <right-terms>\n"
                   "       make-tensor --matmul <p> <n> <m> <k>\n"
                   "       make-tensor --cyclic <p> <length>\n"
                   "       make-tensor --field <p> <modulus coefficients, highest degree first>\n"
                   "       make-tensor --help\n"
                   "\n"
                   "  --polynomial 2 5 5     multiplying two 5-term polynomials over GF(2)\n"
                   "  --matmul 2 2 2 2       <2,2,2>: 2x2 by 2x2 matrices, where Strassen starts\n"
                   "  --cyclic 2 5           multiplying modulo x^5 - 1 over GF(2)\n"
                   "  --field 2 1 1 1        multiplying in GF(2^2), modulus x^2 + x + 1\n"
                   "  --help                 print this and stop, as exit 2\n"
                   "\n"
                   "Writes a tensor file on standard output.";
}

/// The tool proper. main only turns a thrown refusal into a line.
///
/// This is the one command that does not take `cli::Arguments`, and the reason is
/// in the usage above: its four flags are modes rather than settings, and the
/// words after one are its own positional numbers. The walker reads flags and a
/// single positional filename, so `--matmul 2 2 2 2` would hand it four files.
/// What it does take from that header is the refusals: every number below goes
/// through `cli::parse_count`, so a word where a dimension belongs is named with
/// the mode in front of it rather than reported as `stoll` from inside the
/// standard library. `std::stoll(argv[2])` was the worst of those, because it sat
/// outside the try below and left `--matmul abc 2 2 2` as a terminate.
int run(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string mode = argv[1];
    if (mode == "--help" || mode == "-h") {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const int64_t characteristic = cli::parse_whole_number(mode + " <p>", argv[2]);
    const bilinear_rank::Field field(characteristic);

    std::vector<bilinear_rank::Matrix> slices;
    std::string description;

    try {
        if (mode == "--polynomial" && argc == 5) {
            const std::size_t left = cli::parse_count(mode + " <left-terms>", argv[3]);
            const std::size_t right = cli::parse_count(mode + " <right-terms>", argv[4]);
            slices = bilinear_rank::polynomial_multiplication_tensor(left, right);
            description = "Polynomial multiplication of " + std::string(argv[3]) +
                          " coefficients by " + argv[4] + ", over GF(" + argv[2] + ").";
        } else if (mode == "--matmul" && argc == 6) {
            const std::size_t rows = cli::parse_count(mode + " <n>", argv[3]);
            const std::size_t inner = cli::parse_count(mode + " <m>", argv[4]);
            const std::size_t columns = cli::parse_count(mode + " <k>", argv[5]);
            slices = bilinear_rank::matrix_multiplication_tensor(rows, inner, columns);
            description = "Matrix multiplication <" + std::string(argv[3]) + "," + argv[4] + "," +
                          argv[5] + ">, over GF(" + argv[2] + ").";
        } else if (mode == "--cyclic" && argc == 4) {
            const std::size_t length = cli::parse_count(mode + " <length>", argv[3]);
            slices = bilinear_rank::cyclic_convolution_tensor(length);
            description = "Cyclic convolution of length " + std::string(argv[3]) + ", that is "
                          "multiplication modulo x^" + argv[3] + " - 1, over GF(" + argv[2] + ").";
        } else if (mode == "--field" && argc > 4) {
            bilinear_rank::Polynomial modulus;
            for (int argument = 3; argument < argc; ++argument) {
                modulus.push_back(cli::parse_whole_number(mode + " coefficient", argv[argument]));
            }
            slices = bilinear_rank::field_multiplication_tensor(field, modulus);
            description = "Multiplication in GF(" + std::string(argv[2]) + "^" +
                          std::to_string(modulus.size() - 1) + "), modulus given highest degree" +
                          " first.";
        } else {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
    } catch (const cli::ArgumentError&) {
        // Handed up to main, which knows a word on the command line is a usage
        // error. The catch below would report it as a construction that failed.
        throw;
    } catch (const std::exception& problem) {
        // Was 1. This command asks no question, so it has no "no" to give: a
        // construction that threw is a tool that could not run.
        cli::note() << "make-tensor: " << problem.what();
        return cli::exit_status(cli::ExitCode::Error);
    }

    if (slices.empty()) {
        cli::note() << "make-tensor: that produced no slices";
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
    linear_algebra::write_tensor(cli::result(), tensor, comment);
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line that could not be read: nothing was built,
        // so Usage rather than Error.
        cli::note() << "make-tensor: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    }
}
