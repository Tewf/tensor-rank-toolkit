/// Sparsify the operator of a fast multiplication algorithm and report what
/// each method achieved.
///
/// Takes a file and optionally --show flag to control output, enabling scripted
/// runs without interactive prompts.
#include <iostream>
#include <stdexcept>
#include <string>

#include "dense_matrix_file.h"
#include "exit_code.h"
#include "greedy_sparsifier.h"
#include "heuristic_sparsifier.h"
#include "linear_algebra.h"
#include "oracle_sparsifier.h"
#include "sms_file.h"
#include "timing.h"

namespace {

using matrix_sparsification::Field;
using matrix_sparsification::Matrix;

/// Report a result only once it is known to be the same operator. Sparsity is
/// trivial to improve by returning something else entirely.
void report(const Field& field, const std::string& method,
            const Matrix& original, const Matrix& sparsified,
            double seconds, bool show_matrix) {
    const bool equivalent = linear_algebra::same_row_space(field, linear_algebra::transpose<Field>(original),
                                                  linear_algebra::transpose<Field>(sparsified));
    std::cout << "  " << method << ": " << linear_algebra::nonzero_count(field, sparsified)
              << " nonzeros, " << seconds << " s"
              << (equivalent ? "" : "   *** NOT THE SAME OPERATOR ***") << "\n";
    if (show_matrix) std::cout << linear_algebra::to_string(sparsified);
}

/// The tool proper. main only turns a thrown refusal into a line.
int run(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: sparsify-operator <matrix-file|.sms> [--show]\n";
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string path = argv[1];
    const bool show_matrix = (argc > 2 && std::string(argv[2]) == "--show");

    const Field field;
    // The format is inferred from the file extension: `.sms` files use the
    // sparse reader, others use the dense reader. The cost is that an SMS file
    // under any other name silently gets the dense reader, which is one of the
    // failures listed in ../../formats/plinopt_interoperability.md and is
    // waiting on the CLI work.
    const bool is_sms = path.size() > 4 && path.compare(path.size() - 4, 4, ".sms") == 0;
    const Matrix operator_matrix =
        is_sms ? linear_algebra::read_sms_file(path)
               : linear_algebra::read_rational_matrix_file(path);
    const Matrix transposed = linear_algebra::transpose<Field>(operator_matrix);

    std::cout << path << "\n  as given: " << linear_algebra::nonzero_count(field, operator_matrix)
              << " nonzeros, " << operator_matrix.rows() << "x" << operator_matrix.columns()
              << "\n";

    auto started = cli::Clock::now();
    const Matrix sparsifier = matrix_sparsification::row_basis_sparsifier(field, operator_matrix);
    report(field, "row-basis heuristic",
           operator_matrix, linear_algebra::multiply(field, operator_matrix, sparsifier),
           cli::elapsed_seconds(started), show_matrix);

    started = cli::Clock::now();
    const Matrix exhaustive = matrix_sparsification::sparsify_by_best_corank_one(field, transposed);
    report(field, "exact oracle, bottom-up", operator_matrix,
           linear_algebra::transpose<Field>(exhaustive), cli::elapsed_seconds(started), show_matrix);

    started = cli::Clock::now();
    const Matrix top_down = matrix_sparsification::sparsify_by_descending_support(field, transposed);
    report(field, "exact oracle, top-down", operator_matrix,
           linear_algebra::transpose<Field>(top_down), cli::elapsed_seconds(started), show_matrix);

    // `[beniamini2020, Alg. 6]`, which was implemented, tested and then never
    // run from here. It is the one method that minimises `nnz + nns` rather than
    // zeros, and that is where it wins: on the alternative-basis operator all
    // four reach 10 nonzeros, but the oracles leave all ten as ninths, twenty
    // operations, and this leaves ten signs, ten. The line below reports the
    // nonzero count like its siblings, so that column does not separate them;
    // `../README.md` carries the one that does.
    started = cli::Clock::now();
    const Matrix rescaled = matrix_sparsification::sparsify_by_rescaling(field, transposed);
    report(field, "greedy, by rescaling", operator_matrix,
           linear_algebra::transpose<Field>(rescaled), cli::elapsed_seconds(started), show_matrix);

    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& problem) {
        // An unreadable file, or a run that would not fit the memory budget.
        // Reported as a line, not as a terminate. Was 1, which this command has
        // no use for anyway: it sparsifies an operator, it refutes nothing.
        std::cerr << "sparsify-operator: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }
}
