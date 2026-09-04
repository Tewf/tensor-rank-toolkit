/// Sparsify the operator of a fast multiplication algorithm and report the
/// fewest nonzeros any change of basis can leave it with.
///
/// One required file argument; every other option is a flag, listed in
/// `usage()` below.
#include <fstream>
#include <stdexcept>
#include <string>

#include "arguments.h"
#include "dense_matrix_file.h"
#include "exit_code.h"
#include "finite_field_sparsifier.h"
#include "greedy_sparsifier.h"
#include "linear_algebra.h"
#include "memory_budget.h"
#include "lightest_vector_by_simplex.h"
#include "rational_sparsifier.h"
#include "report.h"
#include "sms_file.h"
#include "timing.h"

namespace {

using matrix_sparsification::Field;
using matrix_sparsification::Matrix;

void usage() {
    cli::note() << "usage: sparsify-operator <matrix-file|.sms> [--field p] [--show]\n"
                   "                          [--operations] [--simplex] [--emit out.sms]\n"
                   "                          [--max-memory N]\n"
                   "       sparsify-operator --help\n"
                   "\n"
                   "  Reports the minimum number of nonzeros, and how it was reached.\n"
                   "  Methods that reached the same count more slowly are archived on\n"
                   "  rejected-experiments, retired/dominated_sparsifiers/; the\n"
                   "  measurement that moved them is in ../dominated.md\n"
                   "  --field p       read the entries over GF(p) and answer the question\n"
                   "                  there, exactly, by the matroid greedy over the column\n"
                   "                  space. The routes below work over Q, which is a\n"
                   "                  different and harder question: an operator the rank\n"
                   "                  search emitted is over a finite field and this is the\n"
                   "                  question it is asking. .sms only\n"
                   "  --simplex       answer by linear programming instead of by searching\n"
                   "                  column supports. An upper bound, and the minimum only\n"
                   "                  where the matroid is regular, which no operator measured\n"
                   "                  here is. It reaches the proved minimum anyway wherever\n"
                   "                  the search can prove one, and it is the only route that\n"
                   "                  finishes an operator the search cannot.\n"
                   "                  See method/answering-without-searching.md\n"
                   "  --operations    also run the greedy by rescaling, which minimises\n"
                   "                  nnz + nns rather than nnz: an entry that is neither 0\n"
                   "                  nor +-1 costs a multiplication as well as an addition.\n"
                   "                  A different question, and it costs about 600x more\n"
                   "                  than the answer above, so it is asked for and not\n"
                   "                  assumed. The flag adds that route, not the operations\n"
                   "                  column, which every line carries either way\n"
                   "  --emit PATH     write that minimum as an SMS file, so it can be handed to\n"
                   "                  another tool. Written the way the file came in, so it\n"
                   "                  drops in where the original did: a decoding operator\n"
                   "                  goes back the way up it was given, which is the program\n"
                   "                  that is actually run\n"
                   "  --show          print the sparsified matrix as well as its count\n"
                   "  --max-memory N  bytes one bulk allocation may take, 2G by default. The scan\n"
                   "                  is priced by the column supports it may walk rather than\n"
                   "                  by what it allocates: about ten megabytes on a 23x9\n"
                   "                  operator and 1.4 PiB on a 49x16 one, which is what\n"
                   "                  refuses the second in milliseconds. --simplex answers it\n"
                   "  --help          print this and stop, as exit 2";
}

/// Report a result only once it is known to be the same operator. Sparsity is
/// trivial to improve by returning something else entirely.
///
/// **Both counts, always, because one of them decides nothing.** The routes here
/// reach the same nonzero count on every fixture shipped, so that column
/// separates them not at all: on the alternative-basis operator every route
/// reaches ten, and they are not the same ten. What separates them is
/// `nnz + nns`, the cost the articles minimise, where an entry that is neither
/// `0` nor `±1` costs a multiplication as well as an addition: ten ninths are
/// twenty operations and ten signs are ten. Printing zeros alone showed the
/// routes tying and hid the only result `--operations` exists to produce.
template <class AnyField>
void report(const AnyField& field, const std::string& method,
            const linear_algebra::MatrixOver<AnyField>& original,
            const linear_algebra::MatrixOver<AnyField>& sparsified,
            double seconds, bool show_matrix) {
    const bool equivalent =
        linear_algebra::same_row_space(field, linear_algebra::transpose<AnyField>(original),
                                       linear_algebra::transpose<AnyField>(sparsified));
    cli::result() << "  " << method << ": " << linear_algebra::nonzero_count(field, sparsified)
                  << " nonzeros, " << linear_algebra::operation_count(field, sparsified)
                  << " operations, " << seconds << " s"
                  << (equivalent ? "" : "   *** NOT THE SAME OPERATOR ***") << "\n";
    if (show_matrix) cli::result() << formats::to_string(sparsified);
}

/// The tool proper. main only turns a thrown refusal into a line.
int run(int argc, char** argv) {
    // Walked by `infrastructure/cli/arguments.h` rather than by hand. What the hand-rolled two
    // lines did instead: `--show` was read at argv[2] and nowhere else, so
    // `sparsify-operator m.matrix junk --show` printed no matrix and said nothing
    // about why; every other word on the line was dropped without a message, so a
    // misspelt `--shows` exited 0 having silently ignored the flag; and a second
    // filename was read past in silence, which is what a shell glob hands you
    // when a fixture directory holds more matches than the writer expected.
    // Three ways to ask for something and be told it was done.
    cli::Arguments arguments(argc, argv);
    bool show_matrix = false;
    bool with_operations = false;
    bool by_simplex = false;
    std::string emit_to;
    // Zero means "over Q", which is what every method below assumes and what
    // every fixture here is. A prime asks the other question.
    std::size_t modulus = 0;
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--show")) {
            show_matrix = true;
        } else if (arguments.is("--operations")) {
            with_operations = true;
        } else if (arguments.is("--simplex")) {
            by_simplex = true;
        } else if (arguments.is("--emit")) {
            emit_to = arguments.text();
        } else if (arguments.is("--field")) {
            modulus = arguments.count();
        } else if (arguments.is("--max-memory")) {
            run_limits::set_memory_budget(arguments.memory_size());
        } else {
            arguments.refuse();
        }
    }
    // No file named, and nothing here reads a map on stdin.
    if (arguments.no_file_named()) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string path = arguments.filename();
    const bool is_sms = path.size() > 4 && path.compare(path.size() - 4, 4, ".sms") == 0;

    // **Over a finite field the question is a different one, and it is easy.**
    // `nnz(U V)` over invertible `V` is the total weight of a basis of `U`'s
    // column space, that space has `q^k` elements, and a minimum-weight basis of
    // a matroid is what the greedy returns exactly. The routes below work over
    // `Q`, where the space is infinite and no such walk exists, which is what
    // forces a search over column subsets instead.
    // Every operator the rank strand emits is over a finite field, so this is
    // the question those operators actually pose.
    // [`../finite_field_sparsifier.h`](../finite_field_sparsifier.h).
    if (modulus != 0) {
        if (!is_sms) {
            cli::note() << "--field reads SMS, and " << path
                        << " is not one. The dense reader here is rational: it takes entries "
                           "like 4/9, which no finite field has";
            return cli::exit_status(cli::ExitCode::Usage);
        }
        const linear_algebra::ModularField finite(static_cast<int64_t>(modulus));
        const linear_algebra::ModularMatrix given = formats::read_sms_file(path, finite);
        const bool decoding = given.rows() < given.columns();
        const linear_algebra::ModularMatrix working =
            decoding ? linear_algebra::transpose<linear_algebra::ModularField>(given) : given;

        cli::result() << path << "\n  as given: "
                      << linear_algebra::nonzero_count(finite, given) << " nonzeros, "
                      << linear_algebra::operation_count(finite, given) << " operations, "
                      << given.rows() << "x" << given.columns() << ", over GF(" << modulus << ")\n";
        if (decoding) {
            cli::note() << "wider than tall, so this is a decoding operator: its basis change "
                           "acts on the outputs, which is this question asked of the transpose";
        }
        const auto started_here = cli::Clock::now();
        const linear_algebra::ModularMatrix answer =
            matrix_sparsification::sparsest_basis_over_a_finite_field(finite, working);
        report(finite, "exact, matroid greedy over GF(" + std::to_string(modulus) + ")", working,
               answer, cli::elapsed_seconds(started_here), show_matrix);
        if (!emit_to.empty()) {
            std::ofstream output(emit_to);
            if (!output) throw std::runtime_error("cannot write " + emit_to);
            output << "# minimum-weight basis of " << path << ", by the matroid greedy over GF("
                   << modulus << ")\n";
            formats::write_sms(
                output, decoding ? linear_algebra::transpose<linear_algebra::ModularField>(answer)
                                 : answer);
            cli::note() << "written to " << emit_to;
        }
        return cli::exit_status(cli::ExitCode::Yes);
    }

    const Field field;
    // The format is inferred from the file extension: `.sms` files use the
    // sparse reader, others use the dense reader. The cost is that an SMS file
    // under any other name silently gets the dense reader, which is one of the
    // failures listed in ../../../core/formats/interchange/ and is
    // waiting on the CLI work.
    const Matrix operator_matrix =
        is_sms ? formats::read_sms_file(path)
               : formats::read_rational_matrix_file(path);
    // **A wide operator is a decoding operator, and its basis change is on the
    // other side.** Every method here answers "find invertible `V` minimising
    // `nnz(U V)`", which changes the basis of the space `U`'s columns live in.
    // That is the admissible transform for an encoding operator, `R x n^2`, and
    // it is the wrong one for a decoding operator, `n^2 x R`: there `V` would be
    // `R x R` and would recombine the products themselves, which no change of
    // basis may do and which the other two operators would have to match.
    //
    // The admissible transform for a decoding operator is `W P` with `W`
    // invertible on the output space, and `nnz(W P) = nnz(P^T W^T)` with `W^T`
    // invertible and on the right, so **it is this same question asked of the
    // transpose**. Asked of `P` itself the question is not merely wrong, it is
    // vacuous: every full-rank `n^2 x R` matrix has the whole space as its column
    // space, so the constraint holds for anything and the methods return what
    // they were given.
    //
    // They did exactly that, silently, until 2026-08-22. Every operator fixture
    // shipped here is 7x4, and `tighten-rank-bound --emit-operators` writes `P` the
    // other way up, so the two halves of this repository's own pipeline
    // disagreed about the orientation of the third operator and nothing said so.
    // Measured on a 19-product GF(64) scheme: `P` as emitted, 6x19, reported 54
    // nonzeros from every method in microseconds; the same operator
    // transposed goes to **38**.
    const bool decoding = operator_matrix.rows() < operator_matrix.columns();
    const Matrix working =
        decoding ? linear_algebra::transpose<Field>(operator_matrix) : operator_matrix;
    const Matrix transposed = linear_algebra::transpose<Field>(working);

    cli::result() << path << "\n  as given: "
                  << linear_algebra::nonzero_count(field, operator_matrix) << " nonzeros, "
                  << linear_algebra::operation_count(field, operator_matrix) << " operations, "
                  << operator_matrix.rows() << "x" << operator_matrix.columns() << "\n";
    if (decoding) {
        cli::note() << "wider than tall, so this is a decoding operator: its basis change acts "
                       "on the outputs, which is this question asked of the transpose. Counts "
                       "below are for "
                    << working.rows() << "x" << working.columns()
                    << ", and a nonzero count is the same either way up";
    }

    // **The route that does not search.** Where the operator's matroid is
    // regular this is the minimum and arrives without walking a single column
    // subset; elsewhere it is an upper bound, and it says which it holds.
    if (by_simplex) {
        const auto started_lp = cli::Clock::now();
        const matrix_sparsification::LightestVectors found =
            matrix_sparsification::lightest_vectors_by_simplex(field, transposed);
        if (!found.spans) {
            cli::note() << "the programmes did not produce a spanning set, so there is no "
                           "answer to report";
            return cli::exit_status(cli::ExitCode::Error);
        }
        report(field, "by linear programming", working,
               linear_algebra::transpose<Field>(found.basis),
               cli::elapsed_seconds(started_lp), show_matrix);
        cli::note() << "least weight " << found.least
                    << ", an upper bound. It is the minimum only if this operator's column "
                       "matroid is regular, which is a decision procedure away and is false "
                       "for every operator measured here: see "
                       "methods/matrix_sparsification/method/answering-without-searching.md";
        return cli::exit_status(cli::ExitCode::Yes);
    }

    // **The method that is proved to return the minimum.** `[beniamini2020]`'s
    // own Algorithm 2 with an exact oracle under it, so Rado-Edmonds makes the
    // assembled answer the true minimum over every invertible `V`. The three
    // methods that used to run beside it reached the same count 88x to 343x
    // slower and left on 2026-08-22; `../dominated.md` says where they went.
    // [`../rational_sparsifier.h`](../rational_sparsifier.h).
    auto started = cli::Clock::now();
    const Matrix minimal =
        matrix_sparsification::sparsest_basis_over_the_rationals(field, transposed);
    const Matrix answer = linear_algebra::transpose<Field>(minimal);
    report(field, "exact, matroid greedy over Q", working, answer,
           cli::elapsed_seconds(started), show_matrix);

    if (!emit_to.empty()) {
        std::ofstream output(emit_to);
        if (!output) throw std::runtime_error("cannot write " + emit_to);
        output << "# minimum-weight basis of " << path << ", by the matroid greedy over Q\n";
        // Back the way it came in. The count is the same either way up, but the
        // program is not: a decoding operator written tall computes the
        // transposed map, which is a different straight-line program and a
        // different addition count, and the tool that reads this next has no way
        // to know it was handed the wrong one.
        formats::write_sms(output, decoding ? minimal : answer);
        cli::note() << "written to " << emit_to;
    }
    if (!with_operations) return cli::exit_status(cli::ExitCode::Yes);

    // `[beniamini2020, Alg. 6]`, the one route that minimises `nnz + nns`
    // rather than zeros, and the only reason it survives a comparison it loses
    // on every other axis. On the alternative-basis operator every route reaches
    // 10 nonzeros and they are not the same ten; this one leaves ten signs, ten
    // operations, and guarantees it. The line below reports the nonzero count
    // like its siblings, so that column does not separate them;
    // `../README.md` carries the one that does.
    started = cli::Clock::now();
    const Matrix rescaled = matrix_sparsification::sparsify_by_rescaling(field, transposed);
    report(field, "greedy, by rescaling", working,
           linear_algebra::transpose<Field>(rescaled), cli::elapsed_seconds(started), show_matrix);

    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line that could not be read. The run never
        // started, so Usage and not Error: this used to reach the handler below
        // and leave as 5, reporting a bad flag as a tool that could not run.
        cli::note() << "sparsify-operator: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        // An unreadable file, or a run that would not fit the memory budget.
        // Reported as a line, not as a terminate. Was 1, which this command has
        // no use for anyway: it sparsifies an operator, it refutes nothing.
        cli::note() << "sparsify-operator: " << problem.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
