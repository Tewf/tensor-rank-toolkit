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
#include <stdexcept>
#include <string>

#include "arguments.h"
#include "binary_encoding.h"
#include "exit_code.h"
#include "orbit_cubes.h"
#include "parallel.h"
#include "rank_lower_bound.h"
#include "rank_question.h"
#include "report.h"
#include "types.h"
#include "size_argument.h"
#include "symmetry_argument.h"
#include "tensor_file.h"
#include "tunables.h"

namespace {

void usage() {
    cli::note() << "usage: decide-rank-by-sat <tensor-file>\n"
                   "       decide-rank-by-sat <tensor-file> --target k\n"
                   "       decide-rank-by-sat <tensor-file> --from a --to b\n"
                   "       decide-rank-by-sat --help\n"
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
                   "                      tunables.conf when this is not given. This is the\n"
                   "                      child solver's address space, not this process's\n"
                   "                      allocation budget, which is what the same flag\n"
                   "                      means on decide-rank and minimise-rank\n"
                   "  --threads N         N workers, 0 for every core, 1 by default.\n"
                   "                      -s matmul splits the question into one instance per\n"
                   "                      orbit of the first term, and those are independent\n"
                   "                      solver processes. Each worker's --max-memory is the\n"
                   "                      cap divided by the workers, so the aggregate\n"
                   "                      ceiling is the one number the flag names. A\n"
                   "                      refutation is the same refutation at any count; a\n"
                   "                      yes may come back from a different cube\n"
                   "\n"
                   "  --help              print this and stop, as exit 2\n"
                   "\n"
                   "  exit: 0 yes  1 no  2 usage  3 undecided  4 unverified  5 error\n"
                   "  3 is not 1. A budget that ran out proves nothing either way.";
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
bool report(const formats::Tensor& tensor, std::size_t products,
            const satisfiability::SolveOptions& approach, const std::string& emit_to,
            Progress& progress) {
    if (!emit_to.empty()) {
        // Written first, printed second. Streaming the claim before the call that
        // makes it true reported "wrote <path>" and then threw, for a file that
        // was never created.
        const std::string sizes = satisfiability::write_question(tensor, products, approach, emit_to);
        cli::result() << "  k = " << products << ": wrote " << emit_to << ", " << sizes << "\n";
        return false;
    }

    const satisfiability::Answer answer = satisfiability::decide_rank(tensor, products, approach);
    cli::result() << "  k = " << products << " [" << answer.solver_name << "]: ";
    switch (answer.verdict) {
        case satisfiability::Verdict::Yes:
            cli::result() << "FOUND a decomposition into " << products << "  (" << answer.seconds
                          << " s)\n";
            return true;
        case satisfiability::Verdict::No:
            cli::result() << "NO, rank is more than " << products << "  (" << answer.seconds
                          << " s)";
            if (answer.proof == satisfiability::Proof::Verified) {
                cli::result() << ", refutation verified";
            } else if (answer.proof_bytes > 0) {
                cli::result() << ", refutation " << answer.proof_bytes << " bytes, unchecked";
            }
            cli::result() << "\n";
            return false;
        case satisfiability::Verdict::Unknown:
            cli::result() << "no answer, gave up after " << answer.seconds << " s\n";
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
bool build_orbit_cubes(const formats::Tensor& tensor, const cli::Symmetry& symmetry,
                       std::size_t first_products, satisfiability::SolveOptions& approach) {
    if (symmetry.kind == cli::SymmetryKind::None) return true;

    // Not a limitation to apologise for: an orbit cube *is* a representative of
    // the closed-form orbits of <n,m,k>, so there is no map whose own stabiliser
    // would name one. It is the same refusal `orbit_cubes` already makes.
    if (symmetry.kind == cli::SymmetryKind::Automatic) {
        cli::note() << "decide-rank-by-sat: --symmetry auto has no meaning here. The cubes are "
                       "the\nclosed-form orbits of a matrix multiplication tensor, so name the "
                       "shape:\n--symmetry matmul <n> <m> <k>";
        return false;
    }
    if (tensor.characteristic != 2) {
        cli::note() << "decide-rank-by-sat: --symmetry matmul is GF(2) only, and this tensor is "
                       "over GF(" << tensor.characteristic << ").\nA cube's literals are numbered "
                       "for the Boolean encoding, and the prime field encoder\norders and "
                       "normalises the very term a cube pins, so a refusal would not be a bound";
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
    cli::result() << "  orbit cubes: " << approach.cubes.size()
                  << " instances, one per orbit of the first term\n";
    return true;
}

int run(int argc, char** argv) {
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

    // Walked by `cli/arguments.h` rather than by hand. Five of these are numeric
    // and every one of them used to leave as 5 saying `stoll`, which names the
    // function that threw and none of the five flags it could have been. The
    // hand-written test for a flag where the file belongs goes with the loop: the
    // walker knows a flag from a filename, so `--nonsense` is refused as a flag
    // and `-h` reaches the branch below rather than being read as a path.
    cli::Arguments arguments(argc, argv);
    while (arguments.next_flag()) {
        if (arguments.is("--help", "-h")) {
            usage();
            return cli::exit_status(cli::ExitCode::Usage);
        } else if (arguments.is("--target")) {
            target = arguments.whole_number();
        } else if (arguments.is("--from")) {
            from = arguments.whole_number();
        } else if (arguments.is("--to")) {
            to = arguments.whole_number();
        } else if (arguments.is("--emit-cnf")) {
            emit_to = arguments.text();
        } else if (arguments.is("--backend")) {
            approach.use_field_theory = (arguments.text() == "smt");
        } else if (arguments.is("--solver")) {
            approach.solver = arguments.text();
        } else if (arguments.is("--tune")) {
            const std::string wanted = arguments.text();
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
        } else if (arguments.is("--ceiling")) {
            given_ceiling = arguments.whole_number();
        } else if (arguments.is("--probe")) {
            approach.probe_seconds = arguments.count();
        } else if (arguments.is("--proof")) {
            approach.proof_path = arguments.text();
        } else if (arguments.is("--threads")) {
            // The cubes are independent instances of one formula and
            // `satisfiability/rank_question.cpp` has run them through
            // `parallel_for` since they existed — with the 3.42x it measured
            // written in the comment beside it. Nothing on this command line
            // could reach that call, so `run_limits::worker_count()` was 1 for every run of
            // this tool and the whole split ran one cube at a time. This is the
            // flag it was waiting for.
            run_limits::set_worker_count(arguments.count());
        } else if (arguments.is("--timeout")) {
            approach.timeout_seconds = arguments.count();
        } else if (arguments.is("--max-memory")) {
            approach.memory_megabytes = arguments.memory_size() / (1024 * 1024);
        } else if (arguments.is("--plain-cnf")) {
            approach.plain_cnf = true;
        } else if (arguments.is("--break-symmetry")) {
            approach.break_symmetry = true;
        } else if (arguments.is("--symmetry", "-s")) {
            symmetry = arguments.parsed_by(cli::parse_symmetry);
        } else {
            arguments.refuse();
        }
    }
    // No file named, and nothing here reads a tensor on stdin.
    if (arguments.no_file_named()) {
        usage();
        return cli::exit_status(cli::ExitCode::Usage);
    }
    const std::string path = arguments.filename();

    // A run killed mid-solve leaves its scratch CNF in /tmp, about 300 KB per
    // question, and this is not the file where that is fixed. The name is chosen
    // by `scratch_file` in `satisfiability/solver_process.cpp`, one per question
    // and one per solver log, and it gained a `-<n>` suffix while this was being
    // written; a name predicted from out here would have silently matched nothing
    // and reported a cleanup that never happened, which is worse than the leak.
    // The mechanism is `cli/interrupt_cleanup.h`, and the whole fix is
    // `cli::remove_when_interrupted(file.string())` beside each `scratch_file`
    // call, in the file that knows the name.
    const formats::Tensor tensor = formats::read_tensor_file(path);
    cli::result() << path << ": " << tensor.slices.size() << " slices of " << tensor.rows() << "x"
                  << tensor.columns() << " over GF(" << tensor.characteristic << ")\n";

    // The polynomial bounds cost milliseconds and rule out every rank below
    // them. Asking a solver to refute those is pure waste, and this tool was
    // doing exactly that whenever a sweep started from one. Since `[yang2025]`'s
    // rank sum joined the flattenings this floor is the larger of the two, which
    // on GF(16) is 6 rather than 4 and so skips two solver calls outright.
    const satisfiability::Field field(tensor.characteristic);
    const std::size_t floor = linear_algebra::rank_lower_bound(field, tensor.slices);
    cli::result() << "  lower bound: rank is at least " << floor << "\n";

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
        cli::result() << "  starting at " << floor << " rather than " << from
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
        cli::result() << "  naive upper bound: rank is at most " << ceiling << "\n";
        const auto bounds =
            satisfiability::find_rank(tensor, approach, floor, ceiling);
        cli::result() << "  asked " << bounds.questions_asked << " questions in "
                      << bounds.seconds << " s";
        if (bounds.refutations_verified > 0) {
            cli::result() << ", " << bounds.refutations_verified << " refutations verified";
        }
        cli::result() << "\n";
        if (bounds.exact) {
            cli::result() << "rank is exactly " << bounds.upper << "\n";
            return cli::exit_status(cli::ExitCode::Yes);
        }
        cli::result() << "rank is between " << bounds.lower << " and " << bounds.upper
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
                cli::result() << "rank is exactly " << products
                              << ", since every smaller one was refused\n";
            } else {
                cli::result() << "rank is at most " << products << "\n";
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
        cli::note() << "decide-rank-by-sat: " << problem.what();
        return cli::exit_status(cli::ExitCode::Unverified);
    } catch (const cli::ArgumentError& problem) {
        // A word on the command line, or a line of tunables.conf, that could not
        // be read: the run never started, so Usage rather than Error.
        cli::note() << "decide-rank-by-sat: " << problem.what();
        return cli::exit_status(cli::ExitCode::Usage);
    } catch (const std::exception& problem) {
        cli::note() << "decide-rank-by-sat: " << problem.what();
        return cli::exit_status(cli::ExitCode::Error);
    }
}
