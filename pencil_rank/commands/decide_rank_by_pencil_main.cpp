/// `decide-rank-by-pencil`: the rank of a two-slice tensor, and the reason.
///
/// The other rank commands search and report a number. This one reports the
/// canonical form the number was read off, because that form is the proof: a
/// reader who doubts the rank can check the block sizes add up to the shape and
/// count the nonlinear divisors themselves. Nothing else here can be audited by
/// hand in a minute.
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "arguments.h"
#include "exit_code.h"
#include "kronecker_structure.h"
#include "tensor_file.h"
#include "timing.h"

namespace {

void write_indices(const char* label, const std::vector<std::size_t>& indices) {
    std::cout << "  " << label << ": ";
    if (indices.empty()) {
        std::cout << "none\n";
        return;
    }
    for (std::size_t position = 0; position < indices.size(); ++position) {
        std::cout << (position == 0 ? "" : ", ") << indices[position];
    }
    std::cout << "\n";
}

void write_divisors(const std::vector<pencil_rank::ElementaryDivisor>& divisors) {
    std::cout << "  elementary divisors: ";
    if (divisors.empty()) {
        std::cout << "none\n";
        return;
    }
    for (std::size_t position = 0; position < divisors.size(); ++position) {
        const pencil_rank::PrimePower& factor = divisors[position].factor;
        std::cout << (position == 0 ? "" : ", ");
        if (divisors[position].at_infinity) {
            std::cout << "the point at infinity";
        } else if (factor.base_degree == 1) {
            std::cout << "a root in the field";
        } else {
            std::cout << "an irreducible of degree " << factor.base_degree;
        }
        if (factor.multiplicity > 1) std::cout << " to the power " << factor.multiplicity;
        std::cout << " [degree " << factor.degree() << "]";
    }
    std::cout << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: decide-rank-by-pencil <tensor-file>\n"
                  << "  The Kronecker canonical form of a tensor with at most two slices, and\n"
                  << "  the rank bound it gives. Polynomial time, and no candidate pool.\n";
        return cli::exit_status(cli::ExitCode::Usage);
    }

    // A flag where the tensor should be is a bad invocation, not an unreadable
    // file. Without this, `--route bogus` reports that it cannot read a file
    // called `--route`, which names the wrong thing to fix and exits 5 where a
    // script watching for 2 would not see it.
    if (cli::looks_like_flag(argv[1])) {
        std::cerr << "decide-rank-by-pencil: expected a tensor file, not '" << argv[1] << "'\n";
        return cli::exit_status(cli::ExitCode::Usage);
    }

    try {
        const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(argv[1]);
        if (tensor.slices.size() > 2) {
            std::cerr << "decide-rank-by-pencil: " << tensor.slices.size()
                      << " slices, and this decides pencils. Use decide-rank.\n";
            return cli::exit_status(cli::ExitCode::Usage);
        }

        const pencil_rank::ModularField field(tensor.characteristic);
        const cli::Clock::time_point started = cli::Clock::now();

        const linear_algebra::ModularMatrix second =
            tensor.slices.size() == 2
                ? tensor.slices[1]
                : linear_algebra::ModularMatrix(tensor.rows(), tensor.columns());
        const pencil_rank::KroneckerStructure structure =
            pencil_rank::kronecker_structure(field, tensor.slices[0], second);
        const pencil_rank::PencilRank reported =
            pencil_rank::pencil_rank_of(field, tensor.slices);

        std::cout << "tensor: " << argv[1] << ", GF(" << tensor.characteristic << "), "
                  << tensor.slices.size() << " slices of " << tensor.rows() << "x"
                  << tensor.columns() << "\n";
        std::cout << "Kronecker canonical form:\n";
        write_indices("column minimal indices", structure.column_indices);
        write_indices("row minimal indices", structure.row_indices);
        write_divisors(structure.divisors);
        std::cout << "  regular part: " << structure.regular_size << "x" << structure.regular_size
                  << "\n";

        if (reported.exact) {
            std::cout << "rank: " << reported.proved << " (exact over GF("
                      << tensor.characteristic << "))\n";
        } else {
            std::cout << "rank: at least " << reported.proved << " (proved)\n";
            std::cout << "  over the algebraic closure it is " << reported.over_closure
                      << ", which GF(" << tensor.characteristic << ") can only exceed\n";
            std::cout << "  and likely at least " << reported.over_the_field
                      << " (the same count over GF(" << tensor.characteristic
                      << ") itself, PROVISIONAL)\n";
            std::cout << "  not settled here: this pencil is not diagonalisable over GF("
                      << tensor.characteristic
                      << "), and whether either bound is reached\n"
                      << "  depends on the size of the field. pencil_rank/README.md has the\n"
                      << "  measured gaps.\n";
        }
        std::cerr << "# " << cli::elapsed_seconds(started) << " s\n";

        // A bound is not a rank. `Undecided` rather than `Yes` so that a script
        // sweeping tensors can tell the ones this settled from the ones it only
        // bounded, without parsing the sentence above.
        return cli::exit_status(reported.exact ? cli::ExitCode::Yes : cli::ExitCode::Undecided);
    } catch (const std::exception& error) {
        std::cerr << "decide-rank-by-pencil: " << error.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }
}
