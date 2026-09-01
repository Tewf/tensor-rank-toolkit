/// Two independent derivations of one number, which must agree.
///
/// This is the test that makes either route trustworthy, and it is the one the
/// module never had. The dynamic programme's exactness rested on a single
/// sentence, *"the numbers are small and a dynamic programme settles it"*, and on
/// five hand-computed spot checks; the creation commit `1cbcbe6` argues three
/// other decisions at length and says nothing about it. Now that the same
/// minimisation exists as an integer programme, the two can be run against each
/// other over a sweep, which is the pattern `group_construction` and
/// `test_optimisation.cpp` already use.
///
/// The built-in branch and bound is forced throughout, because it is the only
/// backend whose `Optimal` is a proof. Comparing against a backend whose optimum
/// is unproved would be two claims agreeing rather than two proofs.
#include <iostream>
#include <string>
#include <vector>

#include "check.h"
#include "interpolation_by_solver.h"
#include "interpolation_programme.h"
#include "symmetric_bound_table.h"

namespace {

using curve_bounds::PointSupply;
using curve_bounds::BoundResult;
using curve_bounds::SolverChoice;

std::string named(const std::vector<PointSupply>& supply, std::size_t degree) {
    std::string text = "deg G = " + std::to_string(degree) + " from";
    for (const PointSupply& one : supply) {
        text += " " + std::to_string(one.available) + "x" + std::to_string(one.degree);
    }
    return text;
}

/// The model has to be the same question, not a near relative of it. Four things
/// would each turn it into a different one, and all four are asserted here rather
/// than left to the agreement sweep, where a compensating pair of errors could
/// hide.
void check_the_model_states_the_same_question() {
    const std::vector<PointSupply> supply{{1, 3}, {2, 2}};
    const integer_programme::IntegerProgramme model =
        curve_bounds::interpolation_programme_of(supply, 6);

    check::equal("the objective is minimised",
                 model.sense == integer_programme::Sense::Minimise ? 1 : 0, 1);

    // One equality on the total degree, and never an inequality: a `<=` would
    // always answer "take one rational point, cost 1", a bound on nothing.
    std::size_t equalities = 0;
    std::size_t capacities = 0;
    for (const integer_programme::Constraint& row : model.constraints) {
        if (row.relation == integer_programme::Relation::Equal) ++equalities;
        if (row.relation == integer_programme::Relation::LessOrEqual) ++capacities;
    }
    check::equal("exactly one row fixes the degree", static_cast<long long>(equalities), 1);
    check::equal("one capacity row per supply entry", static_cast<long long>(capacities), 2);
    check::equal("the degree row is an equality on deg G",
                 model.constraints.front().relation == integer_programme::Relation::Equal ? 1 : 0, 1);

    // Every variable is a count: whole, non-negative, and bounded above, because
    // an integer variable with no stated upper bound is binary to CBC and to GLPK.
    bool all_counts = !model.variables.empty();
    for (const integer_programme::Variable& variable : model.variables) {
        if (!variable.integral || !variable.bounded_below || !variable.bounded_above) {
            all_counts = false;
        }
    }
    check::equal("every variable is a bounded whole count", all_counts ? 1 : 0, 1);

    // An unpriced multiplicity is absent as a column, never costed zero.
    // mu_sym_2(2, l) is published for l up to 4 and nothing beyond, and
    // mu_sym_2(1, l) up to 10, so a budget of 6 offers 6 columns for degree 1 and
    // 3 for degree 2.
    check::equal("unpriced multiplicities are absent, not free",
                 static_cast<long long>(model.variables.size()), 9);
    bool priced_only = true;
    for (const integer_programme::Number& price : model.objective) {
        if (price == integer_programme::Number(0)) priced_only = false;
    }
    check::equal("no column costs nothing", priced_only ? 1 : 0, 1);
}

/// `bound` must match. `chosen` need not: ties are real, and the two routes break
/// them differently. What `chosen` must do is re-cost to `bound`, spend exactly
/// the degree, and respect each entry's supply of distinct points.
bool selection_is_consistent(const BoundResult& programme, const std::vector<PointSupply>& supply,
                            std::size_t degree) {
    std::size_t cost = 0;
    std::size_t spent = 0;
    std::vector<std::size_t> taken(supply.size(), 0);
    for (const curve_bounds::Selection& piece : programme.chosen) {
        cost += curve_bounds::symmetric_upper_bound(piece.degree, piece.multiplicity) * piece.count;
        spent += piece.degree * piece.multiplicity * piece.count;
        for (std::size_t entry = 0; entry < supply.size(); ++entry) {
            if (supply[entry].degree == piece.degree) {
                taken[entry] += piece.count;
                break;
            }
        }
    }
    for (std::size_t entry = 0; entry < supply.size(); ++entry) {
        if (taken[entry] > supply[entry].available) return false;
    }
    return cost == programme.bound && spent == degree;
}

/// The sweep. Every supply against every degree it could plausibly be asked for,
/// including the ones with no answer, because agreeing that there is none is as
/// much of an agreement as agreeing on a number.
void check_the_two_routes_agree() {
    const std::vector<std::vector<PointSupply>> supplies{
        {{1, 8}},
        {{1, 3}},
        {{2, 4}},
        {{1, 1}, {2, 4}},
        {{1, 2}, {2, 2}, {3, 2}},
        {{3, 5}},
        {{1, 4}, {4, 3}},
        {{5, 2}, {1, 2}},
        {{11, 5}},
        {{1, 0}},
    };

    std::size_t compared = 0;
    std::size_t feasible = 0;
    std::size_t disagreed = 0;
    for (const std::vector<PointSupply>& supply : supplies) {
        for (std::size_t degree = 1; degree <= 14; ++degree) {
            const BoundResult enumerated = curve_bounds::minimise_interpolation_bound(supply, degree);
            const BoundResult proved = curve_bounds::minimise_interpolation_bound_by_solver(
                supply, degree, SolverChoice::BuiltInOnly);
            ++compared;

            if (enumerated.solved != proved.solved) {
                std::cout << "  FAIL  " << named(supply, degree) << ": one route solved it and the"
                          << " other did not\n";
                ++disagreed;
                continue;
            }
            if (!enumerated.solved) continue;
            ++feasible;
            if (enumerated.bound != proved.bound) {
                std::cout << "  FAIL  " << named(supply, degree) << ": enumeration says "
                          << enumerated.bound << ", branch and bound says " << proved.bound << "\n";
                ++disagreed;
                continue;
            }
            if (!selection_is_consistent(enumerated, supply, degree)) {
                std::cout << "  FAIL  " << named(supply, degree)
                          << ": the enumeration's selection does not re-cost to its bound\n";
                ++disagreed;
            }
            if (!selection_is_consistent(proved, supply, degree)) {
                std::cout << "  FAIL  " << named(supply, degree)
                          << ": the solver's selection does not re-cost to its bound\n";
                ++disagreed;
            }
        }
    }

    std::cout << "  ..    " << compared << " questions compared, " << feasible << " with an answer\n";
    check::equal("the enumeration and the exact solver agree everywhere",
                 static_cast<long long>(disagreed), 0);
    check::equal("and the sweep was not empty", feasible > 60 ? 1 : 0, 1);
}

/// Infeasible and out-of-budget are different answers, and the module has to keep
/// them apart: infeasible is a real result the enumeration returns in O(1), while
/// a budget running out is a reason to ask the other route.
void check_a_refusal_is_not_a_budget_running_out() {
    // Degree 5 cannot be assembled from points of degree 2, at any price.
    const std::vector<PointSupply> even{{2, 4}};
    const BoundResult refused =
        curve_bounds::minimise_interpolation_bound_by_solver(even, 5, SolverChoice::BuiltInOnly);
    check::equal("an impossible degree is refused, not fallen back from",
                 refused.solved ? 1 : 0, 0);
    check::equal("and the refusal is the built-in's, which is the only one believed",
                 refused.solved_by == "built-in" ? 1 : 0, 1);
    check::equal("a refusal is a proof", refused.optimum_proved ? 1 : 0, 1);

    // One node cannot settle anything, so this is Exhausted rather than a verdict,
    // and the enumeration has to take over and say that it did.
    const std::vector<PointSupply> plenty{{1, 4}, {2, 4}, {3, 4}};
    const std::size_t generous = curve_bounds::solver_node_limit();
    curve_bounds::set_solver_node_limit(1);
    const BoundResult fallen =
        curve_bounds::minimise_interpolation_bound_by_solver(plenty, 11, SolverChoice::BuiltInOnly);
    curve_bounds::set_solver_node_limit(generous);

    const BoundResult enumerated = curve_bounds::minimise_interpolation_bound(plenty, 11);
    check::equal("an exhausted budget falls back and still answers", fallen.solved ? 1 : 0,
                 enumerated.solved ? 1 : 0);
    check::equal("with the enumeration's number", static_cast<long long>(fallen.bound),
                 static_cast<long long>(enumerated.bound));
    check::equal("and the fallback is recorded rather than hidden",
                 fallen.solved_by.find("dynamic programme") != std::string::npos ? 1 : 0, 1);
}

/// The chain may be worse and must not be better. Any feasible selection is a
/// valid bound by Theorem 2, so an outside solver stopping early gives a weaker
/// envelope; a *better* number than the proved optimum would mean one of the two
/// is wrong about the model.
void check_the_chain_is_never_better_than_the_proof() {
    const std::vector<std::vector<PointSupply>> supplies{
        {{1, 8}}, {{1, 1}, {2, 4}}, {{1, 2}, {2, 2}, {3, 2}}, {{1, 4}, {4, 3}}};
    std::size_t looser = 0;
    std::size_t impossible = 0;
    for (const std::vector<PointSupply>& supply : supplies) {
        for (std::size_t degree = 3; degree <= 9; ++degree) {
            const BoundResult proved = curve_bounds::minimise_interpolation_bound_by_solver(
                supply, degree, SolverChoice::BuiltInOnly);
            const BoundResult chained = curve_bounds::minimise_interpolation_bound_by_solver(
                supply, degree, SolverChoice::Chain);
            if (proved.solved != chained.solved) {
                std::cout << "  FAIL  " << named(supply, degree)
                          << ": the chain and the proof disagree about whether there is an answer\n";
                ++impossible;
                continue;
            }
            if (!proved.solved) continue;
            if (chained.bound < proved.bound) {
                std::cout << "  FAIL  " << named(supply, degree) << ": the chain returned "
                          << chained.bound << ", below the proved optimum " << proved.bound << "\n";
                ++impossible;
            } else if (chained.bound > proved.bound) {
                ++looser;
            }
        }
    }
    check::equal("the chain never beats the proved optimum", static_cast<long long>(impossible), 0);
    std::cout << "  ..    the chain was looser than the proof on " << looser << " of 28 questions\n";
}

}  // namespace

int main() {
    check_the_model_states_the_same_question();
    check_the_two_routes_agree();
    check_a_refusal_is_not_a_budget_running_out();
    check_the_chain_is_never_better_than_the_proof();
    return check::report("interpolation by solver");
}
