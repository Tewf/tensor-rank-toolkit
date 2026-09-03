/// Håstad's reduction, and Lemma 2's upper bound checked without a solver.
///
/// The shape and the first slice are read off the paper's own worked example,
/// three variables and two clauses, which it prints as 9x9 matrices. The
/// witness is then checked for what Lemma 2 actually claims: at most 4n + 2m
/// matrices, every one of rank at most one, and together they span the tensor.
/// That is a decomposition, and it needs nothing installed to verify.
#include <vector>

#include "boolean_formula.h"
#include "check.h"
#include "formula_to_tensor.h"
#include "measures.h"
#include "span_queries.h"

namespace {

using satisfiability::Clause;
using satisfiability::Field;
using satisfiability::Formula;
using satisfiability::Literal;
using satisfiability::Matrix;

Literal positive(std::size_t variable) { return Literal{variable, false}; }
Literal negative(std::size_t variable) { return Literal{variable, true}; }

/// The paper's example: (x1 or x2 or x3) and (not x1 or not x2 or not x3).
Formula worked_example() {
    Formula formula;
    formula.variable_count = 3;
    formula.clauses = {Clause{{positive(0), positive(1), positive(2)}},
                       Clause{{negative(0), negative(1), negative(2)}}};
    return formula;
}

void check_shape(const Field& field) {
    const Formula formula = worked_example();
    const auto tensor = satisfiability::formula_to_tensor(field, formula);

    check::equal("slices, 3n + m", static_cast<long long>(tensor.slices.size()), 11);
    check::equal("rows, 2 + n + 2m", static_cast<long long>(tensor.rows()), 9);
    check::equal("columns, 3n", static_cast<long long>(tensor.columns()), 9);
    check::equal("target rank, 4n + 2m",
                 static_cast<long long>(satisfiability::target_rank(formula)), 16);

    // V_1 is the paper's first printed matrix: ones at (0,0) and (1,1) only.
    const Matrix& first = tensor.slices[0];
    check::equal("V_1 nonzeros", static_cast<long long>(linear_algebra::nonzero_count(field, first)),
                 2);
    check::equal("V_1 at (0,0)", field.isOne(first(0, 0)) ? 1 : 0, 1);
    check::equal("V_1 at (1,1)", field.isOne(first(1, 1)) ? 1 : 0, 1);

    // V_2 is shifted by two columns, which is what makes the variables
    // independent of each other.
    const Matrix& second = tensor.slices[1];
    check::equal("V_2 at (0,2)", field.isOne(second(0, 2)) ? 1 : 0, 1);
    check::equal("V_2 at (1,3)", field.isOne(second(1, 3)) ? 1 : 0, 1);
}

/// Lemma 2's upper bound: the witness is a decomposition of the tensor.
void check_witness(const Field& field, const Formula& formula, const std::string& what) {
    const auto assignment = satisfiability::satisfying_assignment(formula);
    check::equal(what + " is satisfiable", assignment.found ? 1 : 0, 1);
    if (!assignment.found) return;

    const auto tensor = satisfiability::formula_to_tensor(field, formula);
    const auto witness =
        satisfiability::witness_from_assignment(field, formula, assignment.values);

    const long long bound = static_cast<long long>(satisfiability::target_rank(formula));
    check::equal(what + " witness is within 4n + 2m",
                 static_cast<long long>(witness.size()) <= bound ? 1 : 0, 1);

    std::size_t worst = 0;
    for (const Matrix& piece : witness) {
        const std::size_t rank = linear_algebra::rank(field, piece);
        if (rank > worst) worst = rank;
    }
    check::equal(what + " every witness matrix has rank at most one",
                 static_cast<long long>(worst) <= 1 ? 1 : 0, 1);

    check::equal(what + " the witness spans the tensor",
                 linear_algebra::spans_all(field, witness, tensor.slices) ? 1 : 0, 1);
}

void check_unsatisfiable(const Field& field) {
    // (x) and (not x), padded to three literals each. No assignment works.
    Formula formula;
    formula.variable_count = 1;
    formula.clauses = {Clause{{positive(0)}}, Clause{{negative(0)}}};

    const auto assignment = satisfiability::satisfying_assignment(formula);
    check::equal("an unsatisfiable formula has no assignment", assignment.found ? 1 : 0, 0);

    bool threw = false;
    try {
        satisfiability::witness_from_assignment(field, formula, {true});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check::equal("no witness is built from a non-satisfying assignment", threw ? 1 : 0, 1);
}

}  // namespace

int main() {
    const Field field(2);

    check_shape(field);
    check_witness(field, worked_example(), "worked example");

    // A second formula whose satisfying assignment needs a false variable, so
    // the other arm of M_v is exercised too.
    Formula mixed;
    mixed.variable_count = 2;
    mixed.clauses = {Clause{{negative(0), negative(0), negative(0)}},
                     Clause{{positive(1), negative(0), positive(1)}}};
    check_witness(field, mixed, "false-variable formula");

    check_unsatisfiable(field);
    return check::report("formula to tensor");
}
