#include <iostream>
#include <string>

#include "candidate_pool.h"
#include "canonical_augmentation.h"
#include "exit_code.h"
#include "group_construction.h"
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
    std::cerr << "usage: enumerate-subspaces <tensor-file> --target k -s matmul <n> <m> <k>\n"
                 "\n"
                 "  --target k          the dimension to enumerate up to, which for a\n"
                 "                      solution subspace is the product count\n"
                 "  --plain             orderings deduplicated only, the existing behaviour\n"
                 "  --canonical         McKay canonical augmentation, one per orbit\n"
                 "                      (both passes run when neither is named)\n"
              << cli::symmetry_usage()
              << "\n"
                 "  exit: 0 enumerated  2 usage  4 unverified  5 error\n";
}

void report(const char* name, const bilinear_rank::EnumerationReport& pass) {
    std::cout << "  " << name << ": " << pass.distinct << " distinct subspaces from "
              << pass.emitted << " paths, " << pass.nodes << " nodes, " << pass.group_visits
              << " group visits, " << pass.seconds << " s\n";
}

int run(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string path = argv[1];
    if (path.rfind("--", 0) == 0 || path == "-h") {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(path);
    cli::Symmetry symmetry;
    long long target = -1;
    bool plain = false;
    bool canonical = false;

    for (int argument = 2; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (option == "--target" && argument + 1 < argc) {
            target = std::stoll(argv[++argument]);
        } else if (option == "--plain") {
            plain = true;
        } else if (option == "--canonical") {
            canonical = true;
        } else if (option == "--symmetry" || option == "-s") {
            symmetry = cli::parse_symmetry(argc, argv, argument);
        } else {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
    }
    if (target < 1) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    if (!plain && !canonical) {
        plain = true;
        canonical = true;
    }

    const bilinear_rank::Field field(tensor.characteristic);
    const std::vector<bilinear_rank::Matrix> pool =
        bilinear_rank::all_rank_one_maps(field, tensor.rows(), tensor.columns());
    std::vector<bilinear_rank::Automorphism> group;
    if (symmetry.kind == cli::SymmetryKind::MatrixMultiplication) {
        group = bilinear_rank::matrix_multiplication_symmetries(field, symmetry.shape[0],
                                                               symmetry.shape[1],
                                                               symmetry.shape[2]);
    }
    std::cout << "  pool: " << pool.size() << " rank-one maps, group: " << group.size()
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
        std::cerr << "enumerate-subspaces: " << failure.what() << "\n";
        return cli::exit_status(cli::ExitCode::Unverified);
    } catch (const std::exception& error) {
        std::cerr << "enumerate-subspaces: " << error.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }
}
