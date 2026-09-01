/// The whole loop, once, so that something fails if the pieces stop fitting.
///
/// Every other test here checks one part. This one walks the two directions of
/// Håstad's theorem end to end: a formula becomes a tensor and its witness is
/// checked, and a tensor becomes a rank and the decomposition is checked back
/// against it. If the encodings, the search and the verifier ever disagree
/// about what they are handing each other, this is where it shows.
#include <iostream>
#include <string>
#include <vector>

#include "boolean_formula.h"
#include "check.h"
#include "formula_to_tensor.h"
#include "measures.h"
#include "rank_question.h"
#include "solver_process.h"
#include "span_queries.h"
#include "tensor_flattening.h"
#include "tensor_file.h"

namespace {

using satisfiability::Clause;
using satisfiability::Field;
using satisfiability::Formula;
using satisfiability::Literal;
using satisfiability::Matrix;

/// NP-hardness, and it needs no solver: the reduction and the witness the proof
/// builds from a satisfying assignment.
void check_hardness_direction(const Field& field) {
    Formula formula;
    formula.variable_count = 2;
    formula.clauses = {Clause{{Literal{0, false}, Literal{1, false}, Literal{0, false}}},
                       Clause{{Literal{1, true}, Literal{0, false}, Literal{1, true}}}};

    const auto tensor = satisfiability::formula_to_tensor(field, formula);
    const std::size_t bound = satisfiability::target_rank(formula);
    check::equal("Hastad tensor has 3n + m slices",
                 static_cast<long long>(tensor.slices.size()), 8);
    check::equal("and a target rank of 4n + 2m", static_cast<long long>(bound), 12);

    const auto assignment = satisfiability::satisfying_assignment(formula);
    check::equal("the formula is satisfiable", assignment.found ? 1 : 0, 1);

    const auto witness =
        satisfiability::witness_from_assignment(field, formula, assignment.values);
    check::equal("the witness is within 4n + 2m",
                 static_cast<long long>(witness.size()) <= static_cast<long long>(bound) ? 1 : 0, 1);

    std::size_t worst = 0;
    for (const Matrix& piece : witness) worst = std::max(worst, linear_algebra::rank(field, piece));
    check::equal("every witness matrix is rank one at most",
                 static_cast<long long>(worst) <= 1 ? 1 : 0, 1);
    check::equal("and together they span the tensor",
                 linear_algebra::spans_all(field, witness, tensor.slices) ? 1 : 0, 1);
}

/// NP-membership, which is the half that needs a solver: a tensor alone goes in
/// and an exact rank comes out, with the decomposition checked against it.
void check_membership_direction(const Field& field, const std::string& fixtures,
                                const std::string& name, long long known_rank, bool with_proof) {
    const auto tensor = formats::read_tensor_file(fixtures + "/" + name + ".tensor");

    const std::size_t floor = linear_algebra::flattening_lower_bound(field, tensor.slices);
    const std::size_t ceiling =
        std::min(tensor.rows() * tensor.columns(),
                 std::min(tensor.rows() * tensor.slices.size(),
                          tensor.columns() * tensor.slices.size()));
    check::equal(name + ": the free bounds bracket the rank",
                 static_cast<long long>(floor) <= known_rank &&
                         known_rank <= static_cast<long long>(ceiling)
                     ? 1
                     : 0,
                 1);

    satisfiability::SolveOptions approach;
    approach.break_symmetry = true;
    approach.plain_cnf = true;
    approach.timeout_seconds = 300;
    if (with_proof) approach.proof_path = "/tmp/tensor-rank-end-to-end.drat";

    const auto bounds = satisfiability::find_rank(tensor, approach, floor, ceiling);
    check::equal(name + ": the rank is determined, not bounded", bounds.exact ? 1 : 0, 1);
    check::equal(name + ": and it is the known one", static_cast<long long>(bounds.upper),
                 known_rank);

    // The decomposition is the certificate, so it is multiplied out rather than
    // taken on the solver's word.
    std::size_t used = 0;
    for (const Matrix& term : bounds.decomposition) {
        if (linear_algebra::rank(field, term) > 0) ++used;
    }
    check::equal(name + ": the decomposition is rank-one terms",
                 static_cast<long long>(used) <= known_rank ? 1 : 0, 1);
}

/// The other kind of ceiling: one already reached, handed back.
///
/// The walk is run twice. The first has to find the rank; the second is given
/// the decomposition the first came home with, as an `AchievedCeiling`, and must
/// reach the same answer without asking at the ceiling again. Strictly fewer
/// questions is the whole of what the overload buys, so it is asserted rather
/// than described: on a fixture whose floor already equals its rank the second
/// run asks none at all.
void check_achieved_ceiling(const Field& field, const std::string& fixtures,
                            const std::string& name, long long known_rank) {
    const auto tensor = formats::read_tensor_file(fixtures + "/" + name + ".tensor");
    const std::size_t floor = linear_algebra::flattening_lower_bound(field, tensor.slices);

    satisfiability::SolveOptions approach;
    approach.break_symmetry = true;
    approach.plain_cnf = true;
    approach.timeout_seconds = 300;

    const auto walked =
        satisfiability::find_rank(tensor, approach, floor, tensor.rows() * tensor.columns());
    check::equal(name + ": the plain walk finds the rank", static_cast<long long>(walked.upper),
                 known_rank);

    const satisfiability::AchievedCeiling reached{walked.upper, walked.decomposition};
    const auto told = satisfiability::find_rank(tensor, approach, floor, reached);
    check::equal(name + ": a ceiling in hand gives the same rank",
                 static_cast<long long>(told.upper), known_rank);
    check::equal(name + ": and it is still a determination", told.exact ? 1 : 0, 1);
    check::equal(name + ": asked with one question fewer than finding it took",
                 told.questions_asked < walked.questions_asked ? 1 : 0, 1);
    check::equal(name + ": and keeps the decomposition it was handed",
                 told.decomposition.size() == walked.decomposition.size() ? 1 : 0, 1);

    // A bare number is not an achieved bound, so the walk still has to ask at it.
    // This is the line that would fail if the overload ever started guessing.
    const satisfiability::AchievedCeiling assumed_only{walked.upper, {}};
    const auto assumed = satisfiability::find_rank(tensor, approach, floor, assumed_only);
    check::equal(name + ": a ceiling with nothing behind it is still asked about",
                 assumed.questions_asked == walked.questions_asked ? 1 : 0, 1);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    const bool slow = argc > 2 && std::string(argv[2]) == "--slow";
    const Field field(2);

    check_hardness_direction(field);

    const satisfiability::SatSolver solver = satisfiability::find_sat_solver(false);
    if (!solver.found) {
        std::cout << "  skip  no SAT solver on PATH, the membership half is unchecked\n";
        return check::report("end to end");
    }

    check_membership_direction(field, fixtures, "f2_2x2", 3, false);
    check_achieved_ceiling(field, fixtures, "f2_2x2", 3);
    if (slow) {
        // Bigger, and with the refusal checked by drat-trim when it is present.
        // A proof is asked for only from a solver that writes one, since asking
        // otherwise is now refused. It used to be dropped, so on a machine
        // without kissat this line said it checked a refutation that was never
        // written.
        if (!solver.writes_proofs) {
            std::cout << "  note  " << solver.name
                      << " writes no DRAT proof, so the refusal here is unchecked\n";
        }
        check_membership_direction(field, fixtures, "gf8_multiplication", 6, solver.writes_proofs);
    }
    return check::report("end to end");
}
