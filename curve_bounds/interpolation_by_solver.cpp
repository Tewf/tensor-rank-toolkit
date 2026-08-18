#include "interpolation_by_solver.h"

#include <algorithm>
#include <string>

#include "branch_and_bound.h"
#include "solver_chain.h"
#include "symmetric_bound_table.h"
#include "whole_numbers.h"

namespace curve_bounds {

namespace {

using optimisation::Number;

std::size_t& node_limit_storage() {
    static std::size_t nodes = 200'000;
    return nodes;
}

/// One column: how many points of `supply[entry]`'s degree are taken at this
/// multiplicity. `(degree, multiplicity)` index the variable and not its value,
/// which is what keeps the objective linear: the table lookup happens once here,
/// at encoding time, and becomes a constant coefficient.
struct Column {
    std::size_t entry = 0;
    std::size_t multiplicity = 0;
    std::size_t price = 0;
};

std::vector<Column> columns_of(const std::vector<PointSupply>& supply,
                               std::size_t divisor_degree) {
    std::vector<Column> columns;
    for (std::size_t entry = 0; entry < supply.size(); ++entry) {
        const PointSupply& one = supply[entry];
        if (one.degree == 0 || one.available == 0) continue;
        for (std::size_t multiplicity = 1; multiplicity * one.degree <= divisor_degree;
             ++multiplicity) {
            const std::size_t price = symmetric_upper_bound(one.degree, multiplicity);
            if (price == 0) continue;  // unpriced is absent, never free
            columns.push_back(Column{entry, multiplicity, price});
        }
    }
    return columns;
}

}  // namespace

std::size_t solver_node_limit() { return node_limit_storage(); }
void set_solver_node_limit(std::size_t nodes) { node_limit_storage() = nodes; }

optimisation::IntegerProgramme interpolation_programme_of(const std::vector<PointSupply>& supply,
                                                          std::size_t divisor_degree) {
    optimisation::IntegerProgramme programme;
    programme.sense = optimisation::Sense::Minimise;

    const std::vector<Column> columns = columns_of(supply, divisor_degree);
    for (const Column& column : columns) {
        const PointSupply& one = supply[column.entry];
        // Two ceilings, both real: the supply cannot offer more distinct points
        // than it has, and the budget cannot pay for more than it can afford at
        // this multiplicity. Both bounds are stated because an integer variable
        // with no upper bound is binary to CBC and to GLPK.
        const std::size_t affordable = divisor_degree / (column.multiplicity * one.degree);
        optimisation::Variable variable;
        variable.name = "d" + std::to_string(one.degree) + "u" +
                        std::to_string(column.multiplicity) + "s" + std::to_string(column.entry);
        variable.integral = true;
        variable.bounded_below = true;
        variable.lower = Number(0);
        variable.bounded_above = true;
        variable.upper = Number(static_cast<long>(std::min(one.available, affordable)));
        programme.variables.push_back(variable);
        programme.objective.push_back(Number(static_cast<long>(column.price)));
    }

    // The degree row, an equality. Not a budget: every point costs something, so
    // minimising over smaller divisors too would answer "one rational point,
    // cost 1", which bounds nothing. The roadmap fixes deg G first and then asks
    // for the best divisor of that degree.
    std::vector<optimisation::Coefficient> degree_row;
    for (std::size_t index = 0; index < columns.size(); ++index) {
        const std::size_t weight = columns[index].multiplicity * supply[columns[index].entry].degree;
        degree_row.push_back(
            optimisation::Coefficient{index, Number(static_cast<long>(weight))});
    }
    programme.constraints.push_back(optimisation::Constraint{
        "degG", degree_row, optimisation::Relation::Equal,
        Number(static_cast<long>(divisor_degree))});

    // One row per supply entry: `available` counts distinct points, so raising a
    // point's multiplicity does not buy another point. Multiplicities are not
    // pooled across entries either, which is why the rows are per entry and not
    // per degree.
    for (std::size_t entry = 0; entry < supply.size(); ++entry) {
        if (supply[entry].degree == 0 || supply[entry].available == 0) continue;
        std::vector<optimisation::Coefficient> row;
        for (std::size_t index = 0; index < columns.size(); ++index) {
            if (columns[index].entry != entry) continue;
            row.push_back(optimisation::Coefficient{index, Number(1)});
        }
        if (row.empty()) continue;
        programme.constraints.push_back(optimisation::Constraint{
            "points" + std::to_string(entry), row, optimisation::Relation::LessOrEqual,
            Number(static_cast<long>(supply[entry].available))});
    }
    return programme;
}

Programme minimise_interpolation_bound_by_solver(const std::vector<PointSupply>& supply,
                                                 std::size_t divisor_degree,
                                                 SolverChoice choice) {
    // The degenerate cases the model cannot state: a programme with no columns is
    // not infeasible, it is a question about nothing, and degree 0 is the one the
    // dynamic programme already refuses.
    const optimisation::IntegerProgramme model =
        interpolation_programme_of(supply, divisor_degree);
    if (divisor_degree == 0 || model.variables.empty()) {
        return minimise_interpolation_bound(supply, divisor_degree);
    }

    const optimisation::Solution answer =
        choice == SolverChoice::BuiltInOnly
            ? optimisation::branch_and_bound(model, solver_node_limit())
            : optimisation::solve(model, solver_node_limit());

    // Believed, and it matches the dynamic programme's `solved == false`. Only the
    // built-in ever says it: `solve()` treats an outside solver's infeasibility as
    // one more decline and carries on, so an Infeasible reaching here is exact.
    if (answer.status == optimisation::Status::Infeasible) {
        Programme refused;
        refused.solved_by = answer.solved_by.empty() ? "built-in" : answer.solved_by;
        refused.optimum_proved = true;
        return refused;
    }

    // Exhausted is a budget running out and Unbounded should not occur for a
    // bounded counting programme; neither is an answer, so the dynamic programme
    // takes over. This is where it earns its keep as more than a cross-check.
    //
    // A short `values` is treated the same way. No backend should report Optimal
    // without a point, and reading past the end to find out would be the wrong
    // way to learn that one did.
    if (answer.status != optimisation::Status::Optimal ||
        answer.values.size() < model.variables.size()) {
        Programme fallen_back = minimise_interpolation_bound(supply, divisor_degree);
        fallen_back.solved_by = "dynamic programme, after " +
                                (answer.solved_by.empty() ? std::string("the chain")
                                                          : answer.solved_by) +
                                " was exhausted";
        return fallen_back;
    }

    Programme programme;
    programme.solved = true;
    programme.degree_used = divisor_degree;
    programme.solved_by = answer.solved_by.empty() ? "built-in" : answer.solved_by;
    // The chain sets Optimal on any point passing `satisfies`, which checks
    // bounds, integrality and rows and cannot check optimality. So only the
    // built-in's verdict is a proof, and the difference is recorded rather than
    // smoothed over: a number whose provenance is unrecorded cannot be quoted.
    programme.optimum_proved = programme.solved_by == "built-in";

    const std::vector<Column> columns = columns_of(supply, divisor_degree);
    for (std::size_t index = 0; index < columns.size(); ++index) {
        const long count = static_cast<long>(optimisation::nearest_whole(answer.values[index]));
        if (count <= 0) continue;
        programme.chosen.push_back(Selection{supply[columns[index].entry].degree,
                                             columns[index].multiplicity,
                                             static_cast<std::size_t>(count)});
        programme.bound += columns[index].price * static_cast<std::size_t>(count);
    }
    return programme;
}

}  // namespace curve_bounds
