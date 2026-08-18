/// Constrained minimisation: that the built-in solver is right, and that every
/// solver installed on this machine agrees with it.
#include <sstream>
#include <string>
#include <vector>

#include "branch_and_bound.h"
#include "check.h"
#include "integer_programme.h"
#include "mps_format.h"
#include "simplex.h"
#include "solver_chain.h"
#include "standard_form.h"

namespace {

using optimisation::Constraint;
using optimisation::IntegerProgramme;
using optimisation::Number;
using optimisation::Relation;
using optimisation::Sense;
using optimisation::Solution;
using optimisation::Status;
using optimisation::Variable;

constexpr std::size_t kNodeLimit = 20000;

long long as_integer(const Givaro::Integer& value) {
    std::ostringstream out;
    out << value;
    return std::stoll(out.str());
}

Variable whole_between(int lower, int upper) {
    Variable variable;
    variable.integral = true;
    variable.bounded_below = true;
    variable.lower = Number(lower);
    variable.bounded_above = true;
    variable.upper = Number(upper);
    return variable;
}

Constraint row(std::vector<int> coefficients, Relation relation, Number bound) {
    std::vector<Number> dense;
    for (const int entry : coefficients) dense.push_back(Number(entry));
    return optimisation::constraint_of(dense, relation, bound);
}

std::vector<Number> objective_of(std::vector<int> coefficients) {
    std::vector<Number> objective;
    for (const int entry : coefficients) objective.push_back(Number(entry));
    return objective;
}

/// A programme and the optimum somebody worked out by hand.
struct Case {
    std::string name;
    IntegerProgramme programme;
    long long optimum;
};

std::vector<Case> whole_cases() {
    std::vector<Case> cases;

    IntegerProgramme covering;
    covering.variables = {whole_between(0, 10), whole_between(0, 10)};
    covering.objective = objective_of({1, 1});
    covering.constraints = {row({3, 2}, Relation::GreaterOrEqual, Number(7))};
    cases.push_back({"cover 3x+2y>=7", covering, 3});

    IntegerProgramme knapsack;
    knapsack.sense = Sense::Maximise;
    knapsack.variables = {whole_between(0, 10), whole_between(0, 10), whole_between(0, 10)};
    knapsack.objective = objective_of({5, 4, 3});
    knapsack.constraints = {row({2, 3, 1}, Relation::LessOrEqual, Number(5)),
                            row({4, 1, 2}, Relation::LessOrEqual, Number(11)),
                            row({3, 4, 2}, Relation::LessOrEqual, Number(8))};
    cases.push_back({"knapsack", knapsack, 13});

    IntegerProgramme exact;
    exact.variables = {whole_between(0, 10), whole_between(0, 10)};
    exact.objective = objective_of({1, 1});
    exact.constraints = {row({2, 3}, Relation::Equal, Number(12))};
    cases.push_back({"equality 2x+3y=12", exact, 4});

    // A lower bound that is not zero, which the standard form has to shift away.
    IntegerProgramme shifted;
    shifted.variables = {whole_between(-4, 10)};
    shifted.objective = objective_of({1});
    shifted.constraints = {row({1}, Relation::LessOrEqual, Number(9))};
    cases.push_back({"lower bound at -4", shifted, -4});

    // A variable with no lower bound at all, which has to be split in two.
    IntegerProgramme unbounded_below;
    Variable free_whole;
    free_whole.integral = true;
    free_whole.bounded_below = false;
    unbounded_below.variables = {free_whole, whole_between(0, 1)};
    unbounded_below.objective = objective_of({1, 0});
    unbounded_below.constraints = {row({1, 1}, Relation::Equal, Number(3))};
    cases.push_back({"free variable", unbounded_below, 2});

    return cases;
}

void check_built_in() {
    for (const Case& example : whole_cases()) {
        const Solution answer = optimisation::branch_and_bound(example.programme, kNodeLimit);
        check::equal("built-in solves " + example.name,
                     answer.status == Status::Optimal ? as_integer(answer.objective.nume()) : -999,
                     example.optimum);
        if (!optimisation::satisfies(example.programme, answer.values)) {
            std::cout << "  FAIL  built-in point for " << example.name << " is not feasible\n";
            ++check::failure_count;
        }
    }
}

/// The relaxation on its own, where the answer is a fraction no solver reporting
/// in decimal could hand back exactly.
void check_relaxation() {
    IntegerProgramme continuous;
    continuous.variables = {Variable{}, Variable{}};
    continuous.objective = objective_of({1, 1});
    continuous.constraints = {row({3, 2}, Relation::GreaterOrEqual, Number(7))};

    const optimisation::StandardForm form = optimisation::standard_form_of(continuous);
    const optimisation::LinearOptimum optimum = optimisation::solve_relaxation(form);
    const Number value =
        optimisation::objective_at(continuous, optimisation::original_point(form, optimum.values));
    check::equal("relaxation numerator", as_integer(value.nume()), 7);
    check::equal("relaxation denominator", as_integer(value.deno()), 3);
}

void check_refusals() {
    IntegerProgramme impossible;
    impossible.variables = {whole_between(0, 10)};
    impossible.objective = objective_of({1});
    impossible.constraints = {row({1}, Relation::GreaterOrEqual, Number(3)),
                              row({1}, Relation::LessOrEqual, Number(2))};
    check::equal("infeasible is reported",
                 optimisation::branch_and_bound(impossible, kNodeLimit).status == Status::Infeasible,
                 1);

    IntegerProgramme endless;
    Variable upwards;
    upwards.integral = true;
    endless.variables = {upwards};
    endless.objective = objective_of({-1});
    check::equal("unbounded is reported",
                 optimisation::branch_and_bound(endless, kNodeLimit).status == Status::Unbounded, 1);

    IntegerProgramme rounding;
    rounding.sense = Sense::Maximise;
    Variable capped = whole_between(0, 10);
    rounding.variables = {capped};
    rounding.objective = objective_of({1});
    rounding.constraints = {row({2}, Relation::LessOrEqual, Number(7))};
    const Solution answer = optimisation::branch_and_bound(rounding, kNodeLimit);
    check::equal("maximise under 7/2", as_integer(answer.objective.nume()), 3);

    check::equal("a fraction is not a whole answer",
                 optimisation::satisfies(rounding, {Number(7, 2)}), 0);
}

/// Every solver this machine has, against the same battery. The point is not
/// that they are fast, it is that the chain returns the same answer whichever
/// one of them happens to be installed.
void check_agreement() {
    for (const optimisation::Backend backend : optimisation::available_backends()) {
        const std::string who = optimisation::name_of(backend);
        for (const Case& example : whole_cases()) {
            const Solution answer =
                optimisation::solve_with(backend, example.programme, kNodeLimit);
            const bool usable =
                answer.status == Status::Optimal && optimisation::satisfies(example.programme, answer.values);
            check::equal(who + " on " + example.name,
                         usable ? as_integer(answer.objective.nume()) : -999, example.optimum);
        }
    }
}

void check_chain() {
    const std::vector<optimisation::Backend> present = optimisation::available_backends();
    std::cout << "  note  backends present:";
    for (const optimisation::Backend backend : present) {
        std::cout << " " << optimisation::name_of(backend);
    }
    std::cout << "\n";
    check::equal("the built-in is always available",
                 optimisation::is_available(optimisation::Backend::BuiltIn), 1);
    check::equal("the chain never runs out", present.empty() ? 0 : 1, 1);

    for (const Case& example : whole_cases()) {
        const Solution answer = optimisation::solve(example.programme, kNodeLimit);
        check::equal("chain on " + example.name, as_integer(answer.objective.nume()),
                     example.optimum);
    }

    bool recognised = false;
    optimisation::backend_named("nonesuch", recognised);
    check::equal("an unknown solver name is refused", recognised, 0);
}

/// The two silent traps of the file format, pinned so a rewrite cannot lose them.
void check_written_model() {
    IntegerProgramme programme;
    programme.variables = {whole_between(0, 10)};
    programme.objective = objective_of({1});
    programme.constraints = {row({1}, Relation::GreaterOrEqual, Number(2))};
    std::vector<std::string> written;
    std::istringstream stream(optimisation::mps_of(programme));
    for (std::string line; std::getline(stream, line);) written.push_back(line);

    const auto line_holding = [&](const std::string& text) {
        for (const std::string& line : written) {
            if (line.find(text) != std::string::npos) return line;
        }
        return std::string();
    };

    // Fields three and five begin at columns fifteen and forty. CBC reads them
    // by position however the rest of the file is spaced, so this is the whole
    // difference between a model it understands and one it rejects.
    const std::string marker = line_holding("'INTORG'");
    check::equal("the marker names its section in field three",
                 marker.find("'MARKER'") == 14, 1);
    check::equal("the marker opens the integers in field five",
                 marker.find("'INTORG'") == 39, 1);

    // No default upper bound is ever leaned on: absent one, CBC and GLPK would
    // read this variable as binary and lp_solve would not.
    check::equal("every variable states a lower bound", line_holding("LO BND").find("x1") == 14, 1);
    check::equal("every variable states an upper bound", line_holding("UP BND").find("x1") == 14, 1);
}

}  // namespace

int main() {
    check_built_in();
    check_relaxation();
    check_refusals();
    check_written_model();
    check_agreement();
    check_chain();
    return check::report("optimisation");
}
