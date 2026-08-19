#include <string>

#include "arguments.h"
#include "parallel.h"
#include "candidate_pool.h"
#include "canonical_augmentation.h"
#include "exit_code.h"
#include "group_construction.h"
#include "report.h"
#include "symmetry_argument.h"
#include "tensor_file.h"

/// Counting the solution subspaces, with and without deduplication up to the group.
///
/// The point of the command is the comparison, so it runs both passes unless told
/// otherwise and prints the two node counts beside the two answers. See
/// [`canonical_augmentation.h`](../canonical_augmentation.h) for why the
/// deduplication is McKay's parent test rather than a set of everything seen.
namespace {

void usage() {
    cli::note() << "usage: enumerate-subspaces <tensor-file> --target k -s matmul <n> <m> <k>\n"
                   "\n"
                   "  --target k          the dimension to enumerate up to, which for a\n"
                   "                      solution subspace is the product count\n"
                   "  --plain             orderings deduplicated only, the existing behaviour\n"
                   "  --canonical         McKay canonical augmentation, one per orbit\n"
                   "                      (both passes run when neither is named)\n"
                   "  --threads N         N workers, 0 for every core, 1 by default. This\n"
                   "                      walk counts rather than stops, so every number it\n"
                   "                      reports is the same at any worker count\n"
                   "  --help              print this and stop, as exit 2\n"
                << cli::symmetry_usage()
                << "\n"
                   "  exit: 0 enumerated  2 usage  4 unverified  5 error";
}

void report(const char* name, const bilinear_rank::EnumerationReport& pass) {
    cli::result() << "  " << name << ": " << pass.distinct << " distinct subspaces from "
                  << pass.emitted << " paths, " << pass.nodes << " nodes, " << pass.group_visits
                  << " group visits, " << pass.seconds << " s\n";
}

int run(int argc, char** argv) {
    cli::Symmetry symmetry;
    long long target = -1;
    bool plain = false;
    bool canonical = false;

    // Walked by `cli/arguments.h` rather than by hand, so `--target abc` names the
    // flag and the word rather than leaving as 5 saying `stoll`, and the file is
    // read after the line is understood rather than before: `--help` cannot open
    // a tensor, and a flag where the path belongs is not a missing file.
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--target")) {
            target = arguments.whole_number();
        } else if (arguments.is("--plain")) {
            plain = true;
        } else if (arguments.is("--threads")) {
            bilinear_rank::set_worker_count(arguments.count());
        } else if (arguments.is("--canonical")) {
            canonical = true;
        } else if (arguments.is("--symmetry", "-s")) {
            symmetry = arguments.parsed_by(cli::parse_symmetry);
        } else {
            arguments.refuse();
        }
    }
    // No file named, and nothing here reads a tensor on stdin.
    if (arguments.reads_stdin() || target < 1) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(arguments.filename());

    if (!plain && !canonical) {
        plain = true;
        canonical = true;
    }

    const bilinear_rank::Field field(tensor.characteristic);
    // Considered for `RankOnePool` and deliberately left materialised. Canonical
    // augmentation carries a pool index down its recursion and reads it again on
    // the way back up, when it asks whether the map it added is the canonical
    // parent, so an addressed pool would rebuild the same map once per node. The
    // conversion pays on a single forward scan, which this is not.
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
    std::vector<bilinear_rank::Automorphism> group;
    if (symmetry.kind == cli::SymmetryKind::MatrixMultiplication) {
        // Generators, not the group as a list. Nothing downstream wants the list
        // any more: the parent test canonises by least image and the stabiliser
        // comes from a backtrack, both of which take generators. Asking for the
        // list refused `<3,3,3>` outright at 4 741 632 elements and 6.2 GiB.
        group = bilinear_rank::matrix_multiplication_symmetry_generators(
            field, symmetry.shape[0], symmetry.shape[1], symmetry.shape[2]);
    }
    cli::result() << "  pool: " << pool.size() << " rank-one maps, group: " << group.size()
                  << " elements\n";

    const std::size_t products = static_cast<std::size_t>(target);
    if (plain) {
        report("plain", bilinear_rank::enumerate_solution_subspaces(field, tensor, pool, group,
                                                                   products, false));
    }
    if (canonical) {
        report("canonical", bilinear_rank::enumerate_solution_subspaces(field, tensor, pool, group,
                                                                       products, true));
    }
    return cli::exit_status(cli::ExitCode::Yes);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::CheckFailed& failure) {
        cli::note() << "enumerate-subspaces: " << failure.what();
        return cli::exit_status(cli::ExitCode::Unverified);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line that could not be read: the run never
        // started, so Usage rather than Error. Without this catch every refusal
        // the walker makes would leave as 5, where the loop it replaces left 2.
        cli::note() << "enumerate-subspaces: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& error) {
        cli::note() << "enumerate-subspaces: " << error.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
