#include "symmetry_breaking.h"

namespace satisfiability {

void order_lexicographically(linear_algebra::Cnf& formula, const std::vector<int>& earlier,
                             const std::vector<int>& later) {
    // `equal` tracks "the two agree everywhere so far". While it holds, the
    // earlier term may not carry a 1 where the later one carries a 0.
    int equal = formula.new_variable();
    formula.add_clause({equal});

    for (std::size_t index = 0; index < earlier.size(); ++index) {
        const int mine = earlier[index];
        const int theirs = later[index];
        formula.add_clause({-equal, -mine, theirs});

        if (index + 1 == earlier.size()) break;
        const int still = formula.new_variable();
        formula.add_clause({-still, equal});
        formula.add_clause({-still, -mine, theirs});
        formula.add_clause({-still, mine, -theirs});
        formula.add_clause({still, -equal, mine, theirs});
        formula.add_clause({still, -equal, -mine, -theirs});
        equal = still;
    }
}

void normalise_first_nonzero(linear_algebra::Cnf& formula,
                             const std::vector<std::vector<int>>& groups) {
    if (groups.empty()) return;
    const std::size_t characteristic = groups.front().size();
    if (characteristic < 3) return;  // Over GF(2) there is no scaling to break.

    // `zero_so_far` is true while every entry before this one is zero.
    int zero_so_far = formula.new_variable();
    formula.add_clause({zero_so_far});

    for (std::size_t entry = 0; entry < groups.size(); ++entry) {
        // Still all zero behind us, so this entry is 0 or 1, never more.
        for (std::size_t value = 2; value < characteristic; ++value) {
            formula.add_clause({-zero_so_far, -groups[entry][value]});
        }

        if (entry + 1 == groups.size()) break;
        const int next = formula.new_variable();
        const int is_zero = groups[entry][0];
        formula.add_clause({-next, zero_so_far});
        formula.add_clause({-next, is_zero});
        formula.add_clause({next, -zero_so_far, -is_zero});
        zero_so_far = next;
    }
}

}  // namespace satisfiability
