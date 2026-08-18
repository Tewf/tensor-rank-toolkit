/// Decide the rank by asking a solver, which is what NP-completeness is for.
///
/// `decide-rank` enumerates subspaces and is exponential in a quantity it
/// increases. This states "is there a decomposition into k rank-one terms" once
/// and hands it to a program built for questions of that shape. Same question,
/// different machinery, and the two are expected to agree wherever both finish.
///
/// Sweeping `k` upward from a lower bound gives the rank itself; a single
/// `--target` answers one question, and an unsatisfiable answer at `k` is a
/// proof that the rank exceeds `k`, provided the solver finished.
///
/// Which encoding states the question is [`rank_question.h`](../rank_question.h);
/// this parses arguments and walks the range.
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

#include "arguments.h"
#include "binary_encoding.h"
#include "exit_code.h"
#include "orbit_cubes.h"
#include "rank_lower_bound.h"
#include "rank_question.h"
#include "types.h"
#include "size_argument.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "tunables.h"

namespace {

void usage() {
    std::cerr << "usage: decide-rank-by-sat <tensor-file>\n"
                 "       decide-rank-by-sat <tensor-file> --target k\n"
                 "       decide-rank-by-sat <tensor-file> --from a --to b\n"
                 "\n"
                 "  With no range at all it finds the rank: it sweeps upward from the\n"
                 "  polynomial lower bound to the naive upper bound, and the first k it\n"
                 "  can decompose into is the rank, since every smaller one was refused.\n"
                 "\n"
                 "  --emit-cnf <path>   write the question and stop, for any solver\n"
                 "  --plain-cnf         expand parities into clauses\n"
                 "  --break-symmetry    quotient by term order, and by operand scaling over\n"
                 "                      GF(p). Sound, off by default, and worth at least 76x\n"
                 "                      on a question expected to answer no\n"
                 "  -s, --symmetry matmul <n> <m> <k>\n"
                 "                      split the question into one instance per orbit of the\n"
                 "                      first term: 13 choices instead of 261 121 for <3,3,3>.\n"
                 "                      GF(2) and matrix multiplication tensors only, and\n"
                 "                      'auto' has no meaning here because these orbits are\n"
                 "                      the closed-form ones of <n,m,k> and nothing else\n"
                 "  --backend cnf|smt   cnf encodes the field into clauses (default); smt\n"
                 "                      hands GF(p) to cvc5's theory of finite fields\n"
                 "  --solver <name>     pin a SAT solver instead of taking the best fit\n"
                 "  --tune sat|unsat    kissat's own configurations, for a question whose\n"
                 "                      answer you expect. No default until measured\n"
                 "  --proof <path>      write a DRAT refutation when the answer is no,\n"
                 "                      so the lower bound can be checked independently.\n"
                 "                      kissat only: any other solver, and the smt backend,\n"
                 "                      refuse the flag rather than write nothing\n"
                 "  --ceiling N         override the naive upper bound the search starts from\n"
                 "  --probe N           smaller budget for the questions a search asks on\n"
                 "                      the way, so the full timeout is spent once\n"
                 "  --timeout N         seconds per question, from sat_timeout_seconds\n"
                 "                      in tunables.conf when this is not given\n"
                 "  --max-memory 2G     cap on the solver, from sat_memory_megabytes in\n"
                 "                      tunables.conf when this is not given\n"
                 "\n"
                 "  exit: 0 yes  1 no  2 usage  3 undecided  4 unverified  5 error\n"
                 "  3 is not 1. A budget that ran out proves nothing either way.\n";
}

/// What a sweep has established so far.
struct Progress {
    bool found = false;
    /// True while every `k` below the current one came back a definite no. A
    /// single unknown breaks it, and the answer is then a bound rather than a
    /// determination: the decomposition may have been in the part nobody
    /// finished.
    bool all_below_refused = true;
    /// True once any question came back without an answer. A sweep that never
    /// reaches a yes must then exit `Undecided`, not `No`: some `k` in the range
    /// was never settled either way.
    bool any_undecided = false;
};

/// Ask one `k` and say what came back. True when the sweep should stop.
bool report(const linear_algebra::Tensor& tensor, std::size_t products,
            const satisfiability::SolveOptions& approach, const std::string& emit_to,
            Progress& progress) {
    if (!emit_to.empty()) {
        // Written first, printed second. Streaming the claim before the call that
        // makes it true reported "wrote <path>" and then threw, for a file that
        // was never created.
        const std::string sizes = satisfiability::write_question(tensor, products, approach, emit_to);
        std::cout << "  k = " << products << ": wrote " << emit_to << ", " << sizes << "\n";
        return false;
    }

    const satisfiability::Answer answer = satisfiability::decide_rank(tensor, products, approach);
    std::cout << "  k = " << products << " [" << answer.solver_name << "]: ";
    switch (answer.verdict) {
        case satisfiability::Verdict::Yes:
            std::cout << "FOUND a decomposition into " << products << "  (" << answer.seconds
                      << " s)\n";
            return true;
        case satisfiability::Verdict::No:
            std::cout << "NO, rank is more than " << products << "  (" << answer.seconds << " s)";
            if (answer.proof == satisfiability::Proof::Verified) {
                std::cout << ", refutation verified";
            } else if (answer.proof_bytes > 0) {
                std::cout << ", refutation " << answer.proof_bytes << " bytes, unchecked";
            }
            std::cout << "\n";
            return false;
        case satisfiability::Verdict::Unknown:
            std::cout << "no answer, gave up after " << answer.seconds << " s\n";
            progress.all_below_refused = false;
            progress.any_undecided = true;
            return false;
    }
    return false;
}

/// Fill `approach.cubes` from a `--symmetry` choice. False means refuse.
///
/// The cubes pin the **first** term only, and the encoder allocates each term's
/// operand variables before the next term's, so term 0's numbers are the same
/// whatever `products` is. That is why one encoding gives a numbering good for
/// the whole sweep, and it is asserted in `test_binary_encoding.cpp` rather than
/// left as a reading of the loop, because a change of allocation order would
/// silently pin the wrong variables.
bool build_orbit_cubes(const linear_algebra::Tensor& tensor, const cli::Symmetry& symmetry,
                       std::size_t first_products, satisfiability::SolveOptions& approach) {
    if (symmetry.kind == cli::SymmetryKind::None) return true;

    // Not a limitation to apologise for: an orbit cube *is* a representative of
    // the closed-form orbits of <n,m,k>, so there is no map whose own stabiliser
    // would name one. It is the same refusal `orbit_cubes` already makes.
    if (symmetry.kind == cli::SymmetryKind::Automatic) {
        std::cerr << "decide-rank-by-sat: --symmetry auto has no meaning here. The cubes are the\n"
                     "closed-form orbits of a matrix multiplication tensor, so name the shape:\n"
                     "--symmetry matmul <n> <m> <k>\n";
        return false;
    }
    if (tensor.characteristic != 2) {
        std::cerr << "decide-rank-by-sat: --symmetry matmul is GF(2) only, and this tensor is over"
                     " GF(" << tensor.characteristic << ").\nA cube's literals are numbered for the"
                     " Boolean encoding, and the prime field encoder\norders and normalises the very"
                     " term a cube pins, so a refusal would not be a bound\n";
        return false;
    }

    const satisfiability::BinaryEncoding numbering =
        satisfiability::encode_binary_rank_at_most(tensor, first_products);
    const satisfiability::Field field(tensor.characteristic);
    // The shape is an argument and never inferred from the tensor's dimensions:
    // `orbit_cubes` checks the map really is that product, and that check is the
    // whole guard against pinning the first term to a map this tensor lacks.
    approach.cubes =
        bilinear_rank::orbit_cubes(field, tensor.slices, symmetry.shape[0], symmetry.shape[1],
                                   symmetry.shape[2], numbering.left, numbering.right);
    std::cout << "  orbit cubes: " << approach.cubes.size() << " instances, one per orbit of the"
              << " first term\n";
    return true;
}

int run(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }

    const std::string path = argv[1];
    // A flag where the file belongs is a mistyped command, not a missing file.
    // Reported as usage rather than error so a script can tell them apart.
    if (path.rfind("--", 0) == 0 || path == "-h") {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    satisfiability::SolveOptions approach;
    // The file first, so every flag below overwrites what it said: a flag that
    // was given always wins over tunables.conf, and one that was not leaves the
    // file's number standing.
    const cli::Tunables& tunables = cli::tunables();
    approach.memory_megabytes = tunables.sat_memory_megabytes;
    approach.timeout_seconds = tunables.sat_timeout_seconds;
    approach.solver_order = tunables.sat_solver_order;
    cli::Symmetry symmetry;
    long long target = -1;
    long long from = -1;
    long long to = -1;
    long long given_ceiling = -1;
    std::string emit_to;

    for (int argument = 2; argument < argc; ++argument) {
        const std::string option = argv[argument];
        if (option == "--target" && argument + 1 < argc) {
            target = std::stoll(argv[++argument]);
        } else if (option == "--from" && argument + 1 < argc) {
            from = std::stoll(argv[++argument]);
        } else if (option == "--to" && argument + 1 < argc) {
            to = std::stoll(argv[++argument]);
        } else if (option == "--emit-cnf" && argument + 1 < argc) {
            emit_to = argv[++argument];
        } else if (option == "--backend" && argument + 1 < argc) {
            approach.use_field_theory = (std::string(argv[++argument]) == "smt");
        } else if (option == "--solver" && argument + 1 < argc) {
            approach.solver = argv[++argument];
        } else if (option == "--tune" && argument + 1 < argc) {
            const std::string wanted = argv[++argument];
            if (wanted == "sat") {
                approach.tuning = satisfiability::Tuning::Satisfiable;
            } else if (wanted == "unsat") {
                approach.tuning = satisfiability::Tuning::Unsatisfiable;
            } else if (wanted == "none") {
                approach.tuning = satisfiability::Tuning::None;
            } else {
                usage();
                return cli::exit_status(cli::ExitCode::Usage);
            }
        } else if (option == "--ceiling" && argument + 1 < argc) {
            given_ceiling = std::stoll(argv[++argument]);
        } else if (option == "--probe" && argument + 1 < argc) {
            approach.probe_seconds = cli::parse_count(option, argv[++argument]);
        } else if (option == "--proof" && argument + 1 < argc) {
            approach.proof_path = argv[++argument];
        } else if (option == "--timeout" && argument + 1 < argc) {
            approach.timeout_seconds = cli::parse_count(option, argv[++argument]);
        } else if (option == "--max-memory" && argument + 1 < argc) {
            approach.memory_megabytes =
                cli::parse_memory_size(option, argv[++argument]) / (1024 * 1024);
        } else if (option == "--plain-cnf") {
            approach.plain_cnf = true;
        } else if (option == "--break-symmetry") {
            approach.break_symmetry = true;
        } else if (option == "--symmetry" || option == "-s") {
            symmetry = cli::parse_symmetry(argc, argv, argument);
        } else {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        }
    }

    const linear_algebra::Tensor tensor = linear_algebra::read_tensor_file(path);
    std::cout << path << ": " << tensor.slices.size() << " slices of " << tensor.rows() << "x"
              << tensor.columns() << " over GF(" << tensor.characteristic << ")\n";

    // The polynomial bounds cost milliseconds and rule out every rank below
    // them. Asking a solver to refute those is pure waste, and this tool was
    // doing exactly that whenever a sweep started from one. Since `[yang2025]`'s
    // rank sum joined the flattenings this floor is the larger of the two, which
    // on GF(16) is 6 rather than 4 and so skips two solver calls outright.
    const satisfiability::Field field(tensor.characteristic);
    const std::size_t floor = linear_algebra::rank_lower_bound(field, tensor.slices);
    std::cout << "  lower bound: rank is at least " << floor << "\n";

    // Rank is at most the smallest product of two of the three dimensions:
    // hold one axis fixed and take that many rank-one terms.
    const std::size_t rows = tensor.rows();
    const std::size_t columns = tensor.columns();
    const std::size_t slices = tensor.slices.size();
    std::size_t ceiling = std::min(rows * columns, std::min(rows * slices, columns * slices));
    if (given_ceiling > 0) ceiling = static_cast<std::size_t>(given_ceiling);

    if (target >= 0) from = to = target;
    if (from < 0) from = static_cast<long long>(floor);
    if (static_cast<std::size_t>(from) < floor && target < 0) {
        std::cout << "  starting at " << floor << " rather than " << from
                  << ", which the polynomial bounds already refute\n";
        from = static_cast<long long>(floor);
    }
    if (!build_orbit_cubes(tensor, symmetry, static_cast<std::size_t>(from), approach)) {
        return cli::exit_status(cli::ExitCode::Usage);
    }

    // No range asked for: find the rank by walking up from the flattening bound,
    // which is the one schedule `find_rank` implements. Every question but the
    // last is then a refusal, and the dear ones are the last two, which no
    // schedule can avoid paying.
    if (to < 0 && target < 0 && emit_to.empty()) {
        std::cout << "  naive upper bound: rank is at most " << ceiling << "\n";
        const auto bounds =
            satisfiability::find_rank(tensor, approach, floor, ceiling);
        std::cout << "  asked " << bounds.questions_asked << " questions in " << bounds.seconds
                  << " s";
        if (bounds.refutations_verified > 0) {
            std::cout << ", " << bounds.refutations_verified << " refutations verified";
        }
        std::cout << "\n";
        if (bounds.exact) {
            std::cout << "rank is exactly " << bounds.upper << "\n";
            return cli::exit_status(cli::ExitCode::Yes);
        }
        std::cout << "rank is between " << bounds.lower << " and " << bounds.upper
                  << ", and a question went unanswered\n";
        return cli::exit_status(cli::ExitCode::Undecided);
    }
    if (to < 0) to = static_cast<long long>(ceiling);
    if (to < from) to = from;

    // Whether the sweep began where the flattenings say it must. Starting
    // higher than that leaves ranks untested, so a first success is only a
    // bound rather than the rank.
    Progress progress;
    progress.all_below_refused = static_cast<std::size_t>(from) <= floor;

    for (long long products = from; products <= to; ++products) {
        if (report(tensor, static_cast<std::size_t>(products), approach, emit_to, progress)) {
            if (progress.all_below_refused) {
                std::cout << "rank is exactly " << products
                          << ", since every smaller one was refused\n";
            } else {
                std::cout << "rank is at most " << products << "\n";
            }
            return cli::exit_status(cli::ExitCode::Yes);
        }
    }
    // Writing questions out answers none of them.
    if (!emit_to.empty()) return cli::exit_status(cli::ExitCode::Yes);
    // Nothing in the range was decomposable. That is a refusal only if every
    // question in it actually came back.
    return cli::exit_status(progress.any_undecided ? cli::ExitCode::Undecided : cli::ExitCode::No);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const cli::CheckFailed& problem) {
        // The machine ran and produced something that failed its own check, so
        // this is neither a refusal nor a crash. It means a component is wrong.
        std::cerr << "decide-rank-by-sat: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Unverified);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line, or a line of tunables.conf, that could not
        // be read: the run never started, so Usage rather than Error.
        std::cerr << "decide-rank-by-sat: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        std::cerr << "decide-rank-by-sat: " << problem.what() << "\n";
        return cli::exit_status(cli::ExitCode::Error);
    }
}
