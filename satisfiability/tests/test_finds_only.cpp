/// A solver that can only find, and what the module must and must not believe
/// from it.
///
/// Three claims, each checked without a solver installed, against a stub named
/// yalsat that claims a refutation: the name assigns the class, the class
/// expands the parities and refuses a proof, and a no from it is the third
/// answer. Then, for each real one on `PATH`, the loop end to end on the
/// smallest fixture: a yes that reconstructs, and a no that is never a no.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <unistd.h>

#include "check.h"
#include "dimacs_file.h"
#include "measures.h"
#include "rank_question.h"
#include "solver_process.h"
#include "tensor_file.h"

namespace {

using satisfiability::Field;
using satisfiability::Matrix;
using satisfiability::Verdict;

/// The class is a fact about the solver and not about this machine, so it is
/// asserted by name whether or not the binary is present.
void check_the_name_assigns_the_class(const std::string& stubs) {
    const auto yalsat = satisfiability::find_sat_solver(true, "yalsat");
    check::equal("yalsat can only find", yalsat.finds_only ? 1 : 0, 1);
    check::equal("and reads no x line, whatever was preferred", yalsat.native_xor ? 1 : 0, 0);
    check::equal("and writes no proof", yalsat.writes_proofs ? 1 : 0, 0);
    check::equal("probSAT can only find",
                 satisfiability::find_sat_solver(false, "probSAT").finds_only ? 1 : 0, 1);
    check::equal("multilinear-sat can only find",
                 satisfiability::find_sat_solver(false, "multilinear-sat").finds_only ? 1 : 0, 1);
    check::equal("kissat is complete",
                 satisfiability::find_sat_solver(false, "kissat").finds_only ? 1 : 0, 0);
    check::equal("cryptominisat is complete",
                 satisfiability::find_sat_solver(true, "cryptominisat").finds_only ? 1 : 0, 0);

    const auto by_path = satisfiability::find_sat_solver(false, stubs + "/yalsat");
    check::equal("a path pins a binary that is not on PATH", by_path.found ? 1 : 0, 1);
    check::text("and its name is the file's", by_path.name, "yalsat");
    check::equal("so the class follows the name", by_path.finds_only ? 1 : 0, 1);
    const auto missing = satisfiability::find_sat_solver(false, stubs + "/no_such_solver");
    check::equal("a path to nothing is not found", missing.found ? 1 : 0, 0);
}

linear_algebra::Cnf a_formula_with_parities() {
    linear_algebra::Cnf formula;
    for (int variable = 0; variable < 4; ++variable) formula.new_variable();
    formula.add_clause({1, 2});
    formula.add_parity({1, 2, 3, 4}, true);
    formula.add_parity({2, 3}, false);
    return formula;
}

/// The stub says unsatisfiable and copies what it was handed, so both halves of
/// the class are read off one run: what reached the solver, and what was
/// believed of its answer.
void check_what_the_stub_is_handed_and_believed(const std::string& stubs) {
    const auto stub = satisfiability::find_sat_solver(true, stubs + "/yalsat");
    const linear_algebra::Cnf formula = a_formula_with_parities();

    bool refused = false;
    try {
        satisfiability::run_solver(formula, stub, 256, 5, "/tmp/tensor-rank-finds-only.drat");
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    check::equal("--proof is refused rather than dropped", refused ? 1 : 0, 1);

    const std::string copy =
        "/tmp/tensor-rank-finds-only-" + std::to_string(::getpid()) + ".cnf";
    ::setenv("STUB_SOLVER_COPY", copy.c_str(), 1);
    const auto run = satisfiability::run_solver(formula, stub, 256, 5);
    ::unsetenv("STUB_SOLVER_COPY");
    check::equal("the stub was found and run", run.solver_found ? 1 : 0, 1);
    check::equal("its refutation is the third answer, never a no", run.answered ? 1 : 0, 0);

    std::ifstream handed(copy);
    std::string line;
    std::string header;
    long long x_lines = 0;
    while (std::getline(handed, line)) {
        if (line.rfind("x", 0) == 0) ++x_lines;
        if (line.rfind("p cnf", 0) == 0) header = line;
    }
    std::remove(copy.c_str());
    check::equal("no parity reached it as an x line", x_lines, 0);
    check::text("and the header counts the expanded formula", header,
                "p cnf " + std::to_string(formula.total_variable_count(false)) + " " +
                    std::to_string(formula.total_clause_count(false)));
}

/// A real one, when it is on `PATH`: a yes that reconstructs and a no that is
/// never a no, on the fixture every solver answers in milliseconds.
void check_a_real_one_end_to_end(const std::string& fixtures, const std::string& name) {
    if (!satisfiability::find_sat_solver(false, name).found) {
        std::cout << "  skip  no " << name << " on PATH\n";
        return;
    }
    const Field field(2);
    const auto tensor = linear_algebra::read_tensor_file(fixtures + "/f2_2x2.tensor");
    satisfiability::SolveOptions approach;
    approach.solver = name;
    approach.timeout_seconds = 30;

    const auto found = satisfiability::decide_rank(tensor, 3, approach);
    check::equal(name + ": finds three terms for f2_2x2", found.verdict == Verdict::Yes ? 1 : 0, 1);
    long long used = 0;
    for (const Matrix& term : found.decomposition) {
        if (linear_algebra::rank(field, term) > 0) ++used;
    }
    check::equal(name + ": and hands back at most three rank-one terms", used <= 3 ? 1 : 0, 1);

    // One second is the whole budget for a question with no answer in it.
    approach.timeout_seconds = 1;
    const auto below = satisfiability::decide_rank(tensor, 2, approach);
    check::equal(name + ": two terms is unknown, never no",
                 below.verdict == Verdict::Unknown ? 1 : 0, 1);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string stubs = argc > 1 ? argv[1] : "satisfiability/tests/stub_solvers";
    const std::string fixtures = argc > 2 ? argv[2] : "fixtures";

    check_the_name_assigns_the_class(stubs);
    check_what_the_stub_is_handed_and_believed(stubs);
    for (const char* name : {"yalsat", "probSAT", "multilinear-sat"}) {
        check_a_real_one_end_to_end(fixtures, name);
    }
    return check::report("finds only");
}
