/// `decide-rank-by-pencil`: the rank of a two-slice tensor, and the reason.
///
/// The other rank commands search and report a number. This one reports the
/// canonical form the number was read off, because that form is the proof: a
/// reader who doubts the rank can check the block sizes add up to the shape and
/// count the nonlinear divisors themselves. Nothing else here can be audited by
/// hand in a minute.
#include <exception>
#include <string>
#include <vector>

#include "arguments.h"
#include "exit_code.h"
#include "kronecker_structure.h"
#include "report.h"
#include "tensor_file.h"
#include "timing.h"

namespace {

void usage() {
    cli::note() << "usage: decide-rank-by-pencil <tensor-file>\n"
                   "       decide-rank-by-pencil --help\n"
                   "  The Kronecker canonical form of a tensor with at most two slices, and\n"
                   "  the rank bound it gives. Polynomial time, and no candidate pool.\n"
                   "  There is nothing to tune, so --help is the only flag there is.";
}

void write_indices(const char* label, const std::vector<std::size_t>& indices) {
    cli::result() << "  " << label << ": ";
    if (indices.empty()) {
        cli::result() << "none\n";
        return;
    }
    for (std::size_t position = 0; position < indices.size(); ++position) {
        cli::result() << (position == 0 ? "" : ", ") << indices[position];
    }
    cli::result() << "\n";
}

void write_divisors(const std::vector<pencil_rank::ElementaryDivisor>& divisors) {
    cli::result() << "  elementary divisors: ";
    if (divisors.empty()) {
        cli::result() << "none\n";
        return;
    }
    for (std::size_t position = 0; position < divisors.size(); ++position) {
        const pencil_rank::PrimePower& factor = divisors[position].factor;
        cli::result() << (position == 0 ? "" : ", ");
        if (divisors[position].at_infinity) {
            cli::result() << "the point at infinity";
        } else if (factor.base_degree == 1) {
            cli::result() << "a root in the field";
        } else {
            cli::result() << "an irreducible of degree " << factor.base_degree;
        }
        if (factor.multiplicity > 1) cli::result() << " to the power " << factor.multiplicity;
        cli::result() << " [degree " << factor.degree() << "]";
    }
    cli::result() << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        // Walked by `cli/arguments.h`, which is where the hand-written test for a
        // flag where the tensor belongs went: the walker knows a flag from a
        // filename, so `--route bogus` is refused as a flag this command does not
        // have rather than reported as a file it cannot open, and `--help` reaches
        // the usage instead of the reader.
        cli::Arguments arguments(argc, argv);
        while (arguments.next_flag()) {
            if (arguments.is("--help", "-h")) {
                usage();
                return cli::exit_status(cli::ExitCode::Usage);
            }
            arguments.refuse();
        }
        // No file named, and nothing here reads a tensor on stdin.
        if (arguments.reads_stdin()) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
        const std::string path = arguments.filename();

        const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(path);
        if (tensor.slices.size() > 2) {
            cli::note() << "decide-rank-by-pencil: " << tensor.slices.size()
                        << " slices, and this decides pencils. Use decide-rank.";
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

        cli::result() << "tensor: " << path << ", GF(" << tensor.characteristic << "), "
                      << tensor.slices.size() << " slices of " << tensor.rows() << "x"
                      << tensor.columns() << "\n";
        cli::result() << "Kronecker canonical form:\n";
        write_indices("column minimal indices", structure.column_indices);
        write_indices("row minimal indices", structure.row_indices);
        write_divisors(structure.divisors);
        cli::result() << "  regular part: " << structure.regular_size << "x"
                      << structure.regular_size << "\n";

        if (reported.exact) {
            cli::result() << "rank: " << reported.proved << " (exact over GF("
                          << tensor.characteristic << "))\n";
        } else {
            cli::result() << "rank: at least " << reported.proved << " (proved)\n";
            cli::result() << "  over the algebraic closure it is " << reported.over_closure
                          << ", which GF(" << tensor.characteristic << ") can only exceed\n";
            cli::result() << "  and likely at least " << reported.over_the_field
                          << " (the same count over GF(" << tensor.characteristic
                          << ") itself, PROVISIONAL)\n";
            cli::result() << "  not settled here: this pencil is not diagonalisable over GF("
                          << tensor.characteristic
                          << "), and whether either bound is reached\n"
                          << "  depends on the size of the field. pencil_rank/README.md has the\n"
                          << "  measured gaps.\n";
        }
        cli::note() << cli::elapsed_seconds(started) << " s";

        // A bound is not a rank. `Undecided` rather than `Yes` so that a script
        // sweeping tensors can tell the ones this settled from the ones it only
        // bounded, without parsing the sentence above.
        return cli::exit_status(reported.exact ? cli::ExitCode::Yes : cli::ExitCode::Undecided);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line that could not be read: the run never
        // started, so Usage rather than Error, which is the distinction the test
        // this file used to make by hand was keeping.
        cli::note() << "decide-rank-by-pencil: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& error) {
        cli::note() << "decide-rank-by-pencil: " << error.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
