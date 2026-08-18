/// The transcribed table, and the programme that spends it.
///
/// The table entries checked here are read off `[rambaud2014, Table 1]`, and
/// three of them are checkable a second way: `µ_sym_2(2)`, `µ_sym_2(3)` and
/// `µ_sym_2(4)` are 3, 6 and 9, which are the ranks `bilinear_rank`'s exact
/// search decides for GF(4), GF(8) and GF(16) from the tensors themselves.
#include "check.h"
#include "interpolation_programme.h"
#include "symmetric_bound_table.h"

namespace {

void check_table() {
    // Row l = 1 is multiplication in GF(2^m).
    check::equal("mu_sym_2(1)", static_cast<long long>(curve_bounds::symmetric_upper_bound(1, 1)), 1);
    check::equal("mu_sym_2(2)", static_cast<long long>(curve_bounds::symmetric_upper_bound(2, 1)), 3);
    check::equal("mu_sym_2(3)", static_cast<long long>(curve_bounds::symmetric_upper_bound(3, 1)), 6);
    check::equal("mu_sym_2(4)", static_cast<long long>(curve_bounds::symmetric_upper_bound(4, 1)), 9);
    check::equal("mu_sym_2(5)", static_cast<long long>(curve_bounds::symmetric_upper_bound(5, 1)),
                 13);
    check::equal("mu_sym_2(6)", static_cast<long long>(curve_bounds::symmetric_upper_bound(6, 1)),
                 15);

    check::equal("mu_sym_2(4) is settled",
                 curve_bounds::symmetric_bound(4, 1).settled() ? 1 : 0, 1);

    // m = 7 is 16 - 22: known between, settled at neither end.
    const curve_bounds::Bound seven = curve_bounds::symmetric_bound(7, 1);
    check::equal("mu_sym_2(7) lower", static_cast<long long>(seven.lower), 16);
    check::equal("mu_sym_2(7) upper", static_cast<long long>(seven.upper), 22);
    check::equal("mu_sym_2(7) is not settled", seven.settled() ? 1 : 0, 0);

    // m = 8 has an upper bound only, and a zero lower must not read as a bound.
    const curve_bounds::Bound eight = curve_bounds::symmetric_bound(8, 1);
    check::equal("mu_sym_2(8) upper", static_cast<long long>(eight.upper), 24);
    check::equal("mu_sym_2(8) has no lower bound", static_cast<long long>(eight.lower), 0);
    check::equal("mu_sym_2(8) is not settled", eight.settled() ? 1 : 0, 0);

    // The three the paper sets in bold as new upper bounds.
    check::equal("mu_sym_2(3,2)", static_cast<long long>(curve_bounds::symmetric_upper_bound(3, 2)),
                 16);
    check::equal("mu_sym_2(2,4)", static_cast<long long>(curve_bounds::symmetric_upper_bound(2, 4)),
                 21);
    check::equal("mu_sym_2(1,10)",
                 static_cast<long long>(curve_bounds::symmetric_upper_bound(1, 10)), 30);

    // A dot in the table is unknown, and unknown is not zero-cost.
    check::equal("mu_sym_2(5,2) is unknown",
                 curve_bounds::symmetric_bound(5, 2).known ? 1 : 0, 0);
    check::equal("off the table is unknown",
                 curve_bounds::symmetric_bound(11, 1).known ? 1 : 0, 0);
}

void check_programme() {
    // Rational points only: every point has degree 1, so spending g degree
    // units on g distinct points costs g * mu_sym_2(1,1) = g.
    const curve_bounds::BoundResult rational =
        curve_bounds::minimise_interpolation_bound({{1, 8}}, 5);
    check::equal("rational points solve", rational.solved ? 1 : 0, 1);
    check::equal("rational points bound", static_cast<long long>(rational.bound), 5);
    check::equal("rational points degree", static_cast<long long>(rational.degree_used), 5);

    // With only three points available, the fourth and fifth degree units have
    // to come from raising multiplicities, and mu_sym_2(1,l) is 1, 3, 5, 8, 11
    // -- superadditive, so spreading over points is cheaper than stacking.
    // 5 units on 3 points: 3 + 1 + 1 costs 5 + 1 + 1 = 7.
    const curve_bounds::BoundResult scarce =
        curve_bounds::minimise_interpolation_bound({{1, 3}}, 5);
    check::equal("scarce points bound", static_cast<long long>(scarce.bound), 7);
    check::equal("scarce points degree", static_cast<long long>(scarce.degree_used), 5);

    // A degree-2 point costs 3 for 2 units of degree, while two degree-1 points
    // cost 2 for the same, so the programme must prefer the rational ones when
    // it has them and fall back when it does not.
    const curve_bounds::BoundResult mixed =
        curve_bounds::minimise_interpolation_bound({{1, 1}, {2, 4}}, 5);
    check::equal("mixed supply solves", mixed.solved ? 1 : 0, 1);
    check::equal("mixed supply bound", static_cast<long long>(mixed.bound), 7);

    // No supply at all is not a bound of zero, it is no answer.
    const curve_bounds::BoundResult empty = curve_bounds::minimise_interpolation_bound({}, 5);
    check::equal("no points is unsolved", empty.solved ? 1 : 0, 0);
    check::equal("no points has no bound", static_cast<long long>(empty.bound), 0);

    // Points whose cost the table does not publish cannot be spent.
    const curve_bounds::BoundResult unpriced =
        curve_bounds::minimise_interpolation_bound({{11, 5}}, 20);
    check::equal("unpriceable points are unsolved", unpriced.solved ? 1 : 0, 0);
}

}  // namespace

int main() {
    check_table();
    check_programme();
    return check::report("curve bounds");
}
