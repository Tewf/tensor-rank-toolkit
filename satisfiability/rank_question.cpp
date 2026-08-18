#include "rank_question.h"

#include <fstream>
#include <stdexcept>

#include "binary_encoding.h"
#include "exit_code.h"
#include "field_theory_encoding.h"
#include "model_decomposition.h"
#include "prime_field_encoding.h"
#include "solver_process.h"

namespace satisfiability {

namespace {

/// Refuse the combinations that do not mean anything, by name.
void check_applicable(const linear_algebra::Tensor& tensor, const SolveOptions& approach) {
    if (approach.break_symmetry && approach.use_field_theory) {
        throw std::invalid_argument("the field theory encoding has no ordering constraint");
    }
    // `run_smt_solver` has no proof argument to drop, so a request for one here
    // was lost a layer earlier than the CNF route lost it, and with the same
    // result: a no reported as though something had checked it. cvc5 can emit
    // proofs, in its own format, which nothing here reads.
    if (approach.use_field_theory && !approach.proof_path.empty()) {
        throw std::invalid_argument(
            "the field theory backend writes no DRAT refutation, so a no from it rests on cvc5's "
            "word; --proof is what avoids that, and asking for one here would only look like it");
    }
    // `SolveOptions::cubes` has always said GF(2) only and nothing enforced it. Two
    // ways it goes wrong over a larger prime, both ending in a no that is not a
    // lower bound. A cube is a list of literals numbered for the Boolean
    // encoding, so against a prime encoding numbered differently it pins
    // whichever variables happen to hold those numbers. And
    // `encode_prime_rank_at_most` takes no `first_term_pinned`: it orders term 0
    // against term 1 and normalises term 0's first nonzero entry to 1, neither of
    // which an orbit representative need satisfy. `f2db260` took that collision
    // off the GF(2) side, where the encoder can be told to skip term 0. Here it
    // cannot, so the question is declined rather than answered.
    if (!approach.cubes.empty() || !approach.cube_literals.empty()) {
        if (tensor.characteristic != 2) {
            throw std::invalid_argument(
                "a cube split is GF(2) only: its literals are numbered for the Boolean "
                "encoding, and the prime field encoder orders and normalises the very term a "
                "cube pins, so a refusal from it would not be a lower bound");
        }
        // The SMT route never sees a cube: `from_field_theory` builds its own
        // problem and there is nowhere to put unit clauses numbered for a CNF.
        // Refused rather than ignored, because silently dropping a symmetry
        // request answers a different question from the one that was asked and
        // the two answers look alike.
        if (approach.use_field_theory) {
            throw std::invalid_argument(
                "a cube split and the field theory encoding do not go together: the cube's "
                "literals are CNF variable numbers, which an SMT problem has none of");
        }
    }
}

Answer from_field_theory(const linear_algebra::Tensor& tensor, std::size_t products,
                         const SolveOptions& approach) {
    const auto encoding = encode_field_rank_at_most(tensor, products);
    const auto run =
        run_smt_solver(encoding.problem, approach.memory_megabytes, approach.timeout_seconds);
    if (!run.solver_found) throw std::runtime_error("no cvc5 on PATH");

    Answer answer;
    answer.solver_name = run.solver_name;
    answer.seconds = run.seconds;
    if (!run.answered) return answer;
    if (!run.satisfiable) {
        answer.verdict = Verdict::No;
        return answer;
    }

    const Field field(tensor.characteristic);
    if (!model_reconstructs(field, tensor, encoding, run.field_model)) {
        throw cli::CheckFailed("the model does not reconstruct the tensor");
    }
    answer.verdict = Verdict::Yes;
    answer.decomposition = decomposition_from_model(field, encoding, run.field_model);
    return answer;
}

/// The formula and both readings of a model, built once.
///
/// Held apart from solving so that a cube split derives the formula a single time
/// and appends each cube's unit clauses to a copy, instead of encoding the same
/// question once per candidate.
struct Forms {
    bool binary = false;
    BinaryEncoding boolean_form;
    PrimeFieldEncoding prime_form;

    const linear_algebra::Cnf& formula() const {
        return binary ? boolean_form.formula : prime_form.formula;
    }
};

/// `pinned` says something else has already fixed term 0, which is what a cube
/// does. Read from the same place the unit clauses come from, so the flag and the
/// cube cannot disagree.
Forms build_forms(const linear_algebra::Tensor& tensor, std::size_t products,
                  const SolveOptions& approach, bool pinned) {
    Forms forms;
    forms.binary = tensor.characteristic == 2;
    if (forms.binary) {
        forms.boolean_form =
            encode_binary_rank_at_most(tensor, products, approach.break_symmetry, pinned);
    } else {
        forms.prime_form = encode_prime_rank_at_most(tensor, products, approach.break_symmetry);
    }
    return forms;
}

/// The CNF route, over either field. The two encodings differ in type but not
/// in what happens to them, so the shape below is written once per stage rather
/// than once per field.
Answer answer_from(const linear_algebra::Tensor& tensor, const Forms& forms,
                   const SolveOptions& approach, const std::vector<int>& cube) {
    const bool binary = forms.binary;
    linear_algebra::Cnf formula = forms.formula();
    for (int literal : cube) formula.add_clause({literal});

    const SatSolver solver =
        find_sat_solver(!approach.plain_cnf && !formula.parities.empty(), approach.solver,
                        approach.solver_order);
    const auto run = run_solver(formula, solver, approach.memory_megabytes, approach.timeout_seconds,
                           approach.proof_path, approach.tuning);
    if (!run.solver_found) {
        throw std::runtime_error(
            "no SAT solver on PATH. Install kissat or cryptominisat, or write the question out");
    }

    Answer answer;
    answer.solver_name = run.solver_name;
    answer.seconds = run.seconds;
    answer.proof_bytes = run.proof_bytes;
    answer.proof = run.proof;
    if (!run.answered) return answer;
    if (!run.satisfiable) {
        // A refutation that does not check is not a lower bound. It means the
        // encoding or the solver is wrong, and nothing downstream could tell.
        if (run.proof == Proof::Refuted) {
            throw cli::CheckFailed("the solver's own refutation did not verify");
        }
        answer.verdict = Verdict::No;
        return answer;
    }

    const Field field(tensor.characteristic);
    const bool rebuilt = binary
                             ? model_reconstructs(field, tensor, forms.boolean_form, run.model)
                             : model_reconstructs(field, tensor, forms.prime_form, run.model);
    if (!rebuilt) throw cli::CheckFailed("the model does not reconstruct the tensor");

    answer.verdict = Verdict::Yes;
    answer.decomposition = binary
                               ? decomposition_from_model(field, forms.boolean_form, run.model)
                               : decomposition_from_model(field, forms.prime_form, run.model);
    return answer;
}

Answer from_clauses(const linear_algebra::Tensor& tensor, std::size_t products,
                    const SolveOptions& approach) {
    const Forms forms =
        build_forms(tensor, products, approach, !approach.cube_literals.empty());
    return answer_from(tensor, forms, approach, approach.cube_literals);
}

}  // namespace

Answer decide_rank(const linear_algebra::Tensor& tensor, std::size_t products,
                   const SolveOptions& approach, CubeReport* report) {
    check_applicable(tensor, approach);
    if (approach.use_field_theory) return from_field_theory(tensor, products, approach);
    if (approach.cubes.empty()) return from_clauses(tensor, products, approach);

    // One instance per cube, against one formula: a cube is a list of unit
    // clauses, so the part they share is everything else.
    const Forms forms = build_forms(tensor, products, approach, true);

    // A yes anywhere is a yes; a no needs every cube to refuse, and a single cube
    // that gave up makes the whole answer unknown, because the decomposition may
    // have been in the part nobody finished.
    Answer combined;
    combined.verdict = Verdict::No;
    for (const std::vector<int>& cube : approach.cubes) {
        const Answer piece = answer_from(tensor, forms, approach, cube);
        combined.solver_name = piece.solver_name;
        combined.seconds += piece.seconds;
        combined.proof_bytes += piece.proof_bytes;
        if (report != nullptr) {
            report->verdict.push_back(piece.verdict);
            report->seconds.push_back(piece.seconds);
        }
        if (piece.verdict == Verdict::Yes) {
            combined.verdict = Verdict::Yes;
            combined.decomposition = piece.decomposition;
            return combined;
        }
        if (piece.verdict == Verdict::Unknown) combined.verdict = Verdict::Unknown;
    }
    return combined;
}

namespace {

/// Walk up from the floor to the first satisfiable rank. False if a question
/// went unanswered, which moves no bound: an unknown is not a no, and treating
/// it as one would invent a lower bound.
bool narrow(const linear_algebra::Tensor& tensor, const SolveOptions& approach, std::size_t budget,
            RankBounds& bounds) {
    SolveOptions limited = approach;
    limited.timeout_seconds = budget;

    for (std::size_t k = bounds.lower; k <= bounds.upper; ++k) {
        const Answer answer = decide_rank(tensor, k, limited);
        ++bounds.questions_asked;
        bounds.seconds += answer.seconds;

        if (answer.verdict == Verdict::Unknown) return false;
        if (answer.verdict == Verdict::Yes) {
            bounds.upper = k;
            bounds.decomposition = answer.decomposition;
            return true;
        }
        if (answer.proof == Proof::Verified) ++bounds.refutations_verified;
        bounds.lower = k + 1;
    }
    return true;
}

}  // namespace

RankBounds find_rank(const linear_algebra::Tensor& tensor, const SolveOptions& approach,
                     std::size_t floor, std::size_t ceiling) {
    RankBounds bounds;
    bounds.lower = floor;
    bounds.upper = ceiling;

    // A cheap pass first when one is asked for. Whatever it settles is settled
    // soundly, and whatever it gives up on is left for the pass that can afford
    // it, so the large budget is spent on a bracket rather than on a guess.
    if (approach.probe_seconds > 0 && approach.probe_seconds < approach.timeout_seconds) {
        narrow(tensor, approach, approach.probe_seconds, bounds);

        // A probe that reached the rank has already answered the question, and
        // asking it again at the full budget is a free call spent re-deriving a
        // yes that is in hand. A probe that only ran out of time has settled
        // nothing and the pass below still owes every question from here up.
        if (bounds.lower == bounds.upper && !bounds.decomposition.empty()) {
            bounds.exact = true;
            return bounds;
        }
    }

    bounds.exact = narrow(tensor, approach, approach.timeout_seconds, bounds) &&
                   bounds.lower == bounds.upper;
    return bounds;
}

std::string write_question(const linear_algebra::Tensor& tensor, std::size_t products,
                           const SolveOptions& approach, const std::string& path) {
    check_applicable(tensor, approach);
    // A cube split is one question per cube and this writes one file. Dropping
    // the cubes would write a file that answers a different question, and the
    // difference is invisible in the file, so say so instead.
    if (!approach.cubes.empty()) {
        throw std::invalid_argument("a cube split is " + std::to_string(approach.cubes.size()) +
                                    " questions and this writes one file; ask for them one cube "
                                    "at a time, or write the question without cubes");
    }
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path);

    if (approach.use_field_theory) {
        const auto encoding = encode_field_rank_at_most(tensor, products);
        linear_algebra::write_smtlib(out, encoding.problem);
        return std::to_string(encoding.problem.constants.size()) + " constants, " +
               std::to_string(encoding.problem.assertions.size()) + " assertions";
    }

    const bool binary = tensor.characteristic == 2;
    const linear_algebra::Cnf formula =
        binary ? encode_binary_rank_at_most(tensor, products, approach.break_symmetry).formula
               : encode_prime_rank_at_most(tensor, products, approach.break_symmetry).formula;

    const bool native = !approach.plain_cnf;
    linear_algebra::write_dimacs(out, formula, native);
    return std::to_string(formula.total_variable_count(native)) + " variables, " +
           std::to_string(formula.total_clause_count(native)) + " clauses";
}

}  // namespace satisfiability
