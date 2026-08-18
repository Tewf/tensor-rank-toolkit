#include "branch_and_bound.h"

#include "simplex.h"
#include "standard_form.h"
#include "whole_numbers.h"

namespace optimisation {

namespace {

/// The tree. A node is the whole programme with tighter bounds on one variable,
/// copied rather than shared: the instances this backend exists for are small,
/// and a node that owns its own bounds cannot be corrupted by its sibling.
struct Search {
    const IntegerProgramme& root;
    std::size_t remaining;
    bool have_incumbent = false;
    Number best = Number(0);
    std::vector<Number> best_point;
    bool exhausted = false;
    bool unbounded = false;

    void explore(const IntegerProgramme& node) {
        if (remaining == 0) {
            exhausted = true;
            return;
        }
        --remaining;

        const StandardForm form = standard_form_of(node);
        const LinearOptimum relaxed = solve_relaxation(form);
        if (relaxed.status == Status::Infeasible) return;
        if (relaxed.status == Status::Unbounded) {
            unbounded = true;
            return;
        }

        const std::vector<Number> point = original_point(form, relaxed.values);
        const Number value = objective_at(root, point);
        // The relaxation bounds everything below this node, so a node that
        // cannot beat the incumbent has no descendant that can.
        if (have_incumbent && !improves(root.sense, value, best)) return;

        std::size_t fractional = point.size();
        for (std::size_t index = 0; index < point.size(); ++index) {
            if (node.variables[index].integral && !is_whole(point[index])) {
                fractional = index;
                break;
            }
        }
        if (fractional == point.size()) {
            have_incumbent = true;
            best = value;
            best_point = point;
            return;
        }

        const Givaro::Integer split = floor_of(point[fractional]);

        IntegerProgramme below = node;
        below.variables[fractional].bounded_above = true;
        below.variables[fractional].upper = Number(split);
        explore(below);

        IntegerProgramme above = node;
        above.variables[fractional].bounded_below = true;
        above.variables[fractional].lower = Number(split + 1);
        explore(above);
    }
};

}  // namespace

Solution branch_and_bound(const IntegerProgramme& programme, std::size_t node_limit) {
    Search search{programme, node_limit, false, Number(0), {}, false, false};
    search.explore(programme);

    Solution solution;
    solution.solved_by = "built-in";
    if (search.unbounded) {
        solution.status = Status::Unbounded;
        return solution;
    }
    if (!search.have_incumbent) {
        solution.status = search.exhausted ? Status::Exhausted : Status::Infeasible;
        return solution;
    }
    solution.status = search.exhausted ? Status::Exhausted : Status::Optimal;
    solution.objective = search.best;
    solution.values = search.best_point;
    return solution;
}

}  // namespace optimisation
