/// Read a published algorithm as its three operators and write the map it
/// computes, so a collaborator's files can be an input here without editing.
///
/// This is the direction `--emit-operators` did not have. Everything the field
/// distributes is a *decomposition*, never a tensor: PLinOpt's `data/` is 153
/// SMS operators in `stem_{L,R,P}` triples, and Sedoglavic's catalogue at
/// fmm.univ-lille.fr publishes the same triple as Maple matrices. Nobody ships
/// a matrix-multiplication tensor, because ⟨m,n,k⟩ already determines it. So the
/// `.tensor` format is this repository's own, and the thing that arrives from
/// outside is a triple. `formats/interchange/bringing-an-algorithm-in.md`
/// is the page for whoever has one.
#include <string>
#include <vector>

#include "algorithm_recovery.h"
#include "arguments.h"
#include "exit_code.h"
#include "report.h"
#include "sms_file.h"
#include "tensor_file.h"

namespace {

void usage() {
    cli::note() << "usage: operators-to-tensor <L.sms> <R.sms> <P.sms> --field <p>\n"
                   "       operators-to-tensor --help\n"
                   "\n"
                   "  --field 2, or -q 2     the field to read the operators over\n"
                   "  --help                 print this and stop, as exit 2\n"
                   "\n"
                   "The three files in the order PLinOpt's checkers take them, so\n"
                   "`operators-to-tensor stem_{L,R,P}.sms -q 2` matches\n"
                   "`PMchecker stem_{L,R,P}.sms -q 2` word for word.\n"
                   "\n"
                   "Writes a tensor file on standard output.";
}

/// The bilinear map ⟨L,R,P⟩ computes, which is `map_computed_by` and nothing
/// else: output i is `sum_r P[i][r] * (L[r] . x)(R[r] . y)`, so slice i is
/// `sum_r P[i][r] * L[r] (x) R[r]`. That is PLinOpt's convention as well as this
/// repository's, which is asserted rather than assumed in
/// `methods/bilinear_rank/greedy_heuristic/tests/test_operators_to_tensor.cpp`.
formats::Tensor map_of(const bilinear_rank::Field& field, int64_t characteristic,
                              const bilinear_rank::Algorithm& algorithm) {
    formats::Tensor tensor;
    tensor.characteristic = characteristic;
    tensor.slices = bilinear_rank::map_computed_by(field, algorithm);
    return tensor;
}

/// PLinOpt refuses the same disagreement, in the same words: "inner dimension
/// mismatch". A triple whose shapes do not meet is not a slow answer, it is
/// three files that were never one algorithm.
void require_shapes_meet(const bilinear_rank::Algorithm& algorithm) {
    if (algorithm.left.rows() == algorithm.right.rows() &&
        algorithm.left.rows() == algorithm.decode.columns()) {
        return;
    }
    throw std::runtime_error(
        "inner dimension mismatch: L has " + std::to_string(algorithm.left.rows()) +
        " rows, R has " + std::to_string(algorithm.right.rows()) + ", and P has " +
        std::to_string(algorithm.decode.columns()) + " columns. All three are the product count");
}

int run(int argc, char** argv) {
    // Three positional files rather than the one `cli::Arguments` collects, for
    // the reason in the usage: the argument shape is his checkers', not ours.
    std::vector<std::string> files;
    int64_t characteristic = 0;
    for (int at = 1; at < argc; ++at) {
        const std::string word = argv[at];
        if (word == "--help" || word == "-h") {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
        if (word == "--field" || word == "-q") {
            if (at + 1 >= argc) throw cli::ArgumentError(word + " needs a value");
            characteristic = cli::parse_whole_number(word, argv[++at]);
        } else if (cli::looks_like_flag(word)) {
            throw cli::ArgumentError("unrecognised option: " + word);
        } else {
            files.push_back(word);
        }
    }
    if (files.size() != 3) {
        cli::note() << "operators-to-tensor reads three files, L then R then P, and was given "
                    << files.size();
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    if (!linear_algebra::is_prime(characteristic)) {
        throw cli::ArgumentError(
            "--field expects a prime, not '" + std::to_string(characteristic) +
            "'. SMS carries no field, so there is nothing to fall back on: PLinOpt takes it "
            "on the command line too, as -q");
    }

    const bilinear_rank::Field field(characteristic);
    const bilinear_rank::Algorithm algorithm{formats::read_sms_file(files[0], field),
                                             formats::read_sms_file(files[1], field),
                                             formats::read_sms_file(files[2], field)};
    require_shapes_meet(algorithm);

    const formats::Tensor tensor = map_of(field, characteristic, algorithm);
    cli::note() << "read " << algorithm.product_count() << " products over GF(" << characteristic
                << "): L is " << algorithm.left.rows() << "x" << algorithm.left.columns()
                << ", R is " << algorithm.right.rows() << "x" << algorithm.right.columns()
                << ", P is " << algorithm.decode.rows() << "x" << algorithm.decode.columns();
    formats::write_tensor(cli::result(), tensor,
                                 "The map computed by " + files[0] + ", " + files[1] + " and " +
                                     files[2] + ",\nread by operators-to-tensor over GF(" +
                                     std::to_string(characteristic) + ") from " +
                                     std::to_string(algorithm.product_count()) + " products.");
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::ArgumentError& problem) {
        cli::note() << "operators-to-tensor: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        cli::note() << "operators-to-tensor: " << problem.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
