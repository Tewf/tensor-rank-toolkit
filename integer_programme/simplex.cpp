#include "simplex.h"

namespace optimisation {

namespace {

/// Rows in reduced form against the current basis: entry `[row][columns]` is the
/// right-hand side, and `basis[row]` is the column standing at one there.
struct Tableau {
    std::vector<std::vector<Number>> rows;
    std::vector<std::size_t> basis;
    std::size_t columns = 0;
};

void pivot(Tableau& tableau, std::size_t row, std::size_t column) {
    const Number scale = tableau.rows[row][column];
    for (Number& entry : tableau.rows[row]) entry /= scale;
    for (std::size_t other = 0; other < tableau.rows.size(); ++other) {
        if (other == row) continue;
        const Number factor = tableau.rows[other][column];
        if (factor == Number(0)) continue;
        for (std::size_t index = 0; index <= tableau.columns; ++index) {
            tableau.rows[other][index] -= factor * tableau.rows[row][index];
        }
    }
    tableau.basis[row] = column;
}

/// The reduced cost of a column: what entering it would pay, once the basis has
/// been made to compensate. Recomputed rather than carried in an objective row,
/// because exact arithmetic makes the recomputation cheap enough and a carried
/// row is one more thing to get out of step.
Number reduced_cost(const Tableau& tableau, const std::vector<Number>& cost, std::size_t column) {
    Number total = cost[column];
    for (std::size_t row = 0; row < tableau.rows.size(); ++row) {
        total -= cost[tableau.basis[row]] * tableau.rows[row][column];
    }
    return total;
}

/// `[bland1977, Thm. 1.1]`'s Rule I on both choices, which is what buys the
/// finiteness: the first column that pays, and among the rows
/// that bound it the one whose basic column has the smallest index.
Status optimise(Tableau& tableau, const std::vector<Number>& cost) {
    for (;;) {
        std::size_t entering = tableau.columns;
        for (std::size_t column = 0; column < tableau.columns; ++column) {
            if (reduced_cost(tableau, cost, column) < Number(0)) {
                entering = column;
                break;
            }
        }
        if (entering == tableau.columns) return Status::Optimal;

        std::size_t leaving = tableau.rows.size();
        Number tightest = Number(0);
        for (std::size_t row = 0; row < tableau.rows.size(); ++row) {
            const Number& coefficient = tableau.rows[row][entering];
            if (coefficient <= Number(0)) continue;
            const Number ratio = tableau.rows[row][tableau.columns] / coefficient;
            const bool better = leaving == tableau.rows.size() || ratio < tightest ||
                                (ratio == tightest && tableau.basis[row] < tableau.basis[leaving]);
            if (better) {
                tightest = ratio;
                leaving = row;
            }
        }
        if (leaving == tableau.rows.size()) return Status::Unbounded;
        pivot(tableau, leaving, entering);
    }
}

/// One artificial column per row, which is a feasible basis because every bound
/// is non-negative. Its cheapest total is zero exactly when the real columns can
/// carry the rows unaided.
Tableau with_artificials(const StandardForm& form) {
    Tableau tableau;
    tableau.columns = form.columns + form.rows.size();
    tableau.rows.assign(form.rows.size(), {});
    tableau.basis.resize(form.rows.size());
    for (std::size_t row = 0; row < form.rows.size(); ++row) {
        tableau.rows[row].assign(tableau.columns + 1, Number(0));
        for (std::size_t column = 0; column < form.columns; ++column) {
            tableau.rows[row][column] = form.rows[row][column];
        }
        tableau.rows[row][form.columns + row] = Number(1);
        tableau.rows[row][tableau.columns] = form.bound[row];
        tableau.basis[row] = form.columns + row;
    }
    return tableau;
}

/// The same rows without the artificial columns, dropping any row still standing
/// on one. Such a row is zero across every real column with a zero bound, which
/// is to say it was redundant; keeping it would let phase two pivot an
/// artificial back up off zero and quietly leave the feasible set.
Tableau without_artificials(const Tableau& tableau, std::size_t real) {
    Tableau shrunk;
    shrunk.columns = real;
    for (std::size_t row = 0; row < tableau.rows.size(); ++row) {
        if (tableau.basis[row] >= real) continue;
        std::vector<Number> kept(real + 1, Number(0));
        for (std::size_t column = 0; column < real; ++column) kept[column] = tableau.rows[row][column];
        kept[real] = tableau.rows[row][tableau.columns];
        shrunk.rows.push_back(std::move(kept));
        shrunk.basis.push_back(tableau.basis[row]);
    }
    return shrunk;
}

}  // namespace

LinearOptimum solve_relaxation(const StandardForm& form) {
    Tableau tableau = with_artificials(form);

    std::vector<Number> phase_one(tableau.columns, Number(0));
    for (std::size_t row = 0; row < form.rows.size(); ++row) phase_one[form.columns + row] = Number(1);
    optimise(tableau, phase_one);

    Number residual = Number(0);
    for (std::size_t row = 0; row < tableau.rows.size(); ++row) {
        if (tableau.basis[row] >= form.columns) residual += tableau.rows[row][tableau.columns];
    }
    if (residual > Number(0)) return {Status::Infeasible, {}};

    // An artificial sitting at zero can usually be swapped for a real column in
    // the same row; the rows where it cannot are the redundant ones, dropped next.
    for (std::size_t row = 0; row < tableau.rows.size(); ++row) {
        if (tableau.basis[row] < form.columns) continue;
        for (std::size_t column = 0; column < form.columns; ++column) {
            if (tableau.rows[row][column] != Number(0)) {
                pivot(tableau, row, column);
                break;
            }
        }
    }

    Tableau second = without_artificials(tableau, form.columns);
    if (optimise(second, form.cost) == Status::Unbounded) return {Status::Unbounded, {}};

    std::vector<Number> values(form.columns, Number(0));
    for (std::size_t row = 0; row < second.rows.size(); ++row) {
        values[second.basis[row]] = second.rows[row][form.columns];
    }
    return {Status::Optimal, std::move(values)};
}

}  // namespace optimisation
