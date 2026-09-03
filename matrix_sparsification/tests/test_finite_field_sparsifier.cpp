/// That the greedy over a finite field really is exact, checked against the only
/// authority that settles it: every basis, enumerated.
///
/// The claim is a theorem: linear independence is a matroid, and the greedy
/// returns a minimum-weight basis of any matroid under any weight function.
/// But a theorem is about the algorithm and a test is about the code, and the
/// two have disagreed here before. So the small cases are brute-forced: every
/// `k`-subset of the column space is tried, the lightest independent one wins,
/// and the greedy has to match it exactly rather than approach it.
///
/// **What is asserted, in order of how much it would cost to get wrong.** That
/// the answer spans the same column space, because sparsity is trivial to
/// improve by returning a different matrix. That its weight equals the brute
/// force optimum. And that it never comes back heavier than what it was given,
/// which is the one failure a caller would not notice.
#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

#include "check.h"
#include "field.h"
#include "finite_field_sparsifier.h"
#include "linear_algebra.h"
#include "matrix.h"
#include "measures.h"

namespace {

using linear_algebra::ModularField;
using linear_algebra::ModularMatrix;

/// The lightest basis of the column space, by trying every one of them.
std::size_t lightest_basis_by_brute_force(const ModularField& field, const ModularMatrix& given) {
    const std::size_t rows = given.rows();
    const std::size_t width = given.columns();
    const std::size_t modulus = static_cast<std::size_t>(field.residu());

    std::size_t count = 1;
    for (std::size_t taken = 0; taken < width; ++taken) count *= modulus;

    std::vector<std::vector<int64_t>> space;
    for (std::size_t code = 1; code < count; ++code) {
        std::vector<int64_t> column(rows, 0);
        std::size_t left = code;
        for (std::size_t source = 0; source < width; ++source) {
            const std::size_t coefficient = left % modulus;
            left /= modulus;
            if (coefficient == 0) continue;
            for (std::size_t row = 0; row < rows; ++row) {
                int64_t scaled = 0;
                field.init(scaled, static_cast<int64_t>(coefficient));
                field.axpyin(column[row], scaled, given(row, source));
            }
        }
        space.push_back(column);
    }

    // Every subset of the right size, which is what "every basis" means here.
    std::size_t best = static_cast<std::size_t>(-1);
    std::vector<std::size_t> chosen(width);
    const std::function<void(std::size_t, std::size_t)> walk = [&](std::size_t from,
                                                                   std::size_t taken) {
        if (taken == width) {
            ModularMatrix trial(rows, width);
            std::size_t weight = 0;
            for (std::size_t column = 0; column < width; ++column) {
                for (std::size_t row = 0; row < rows; ++row) {
                    trial(row, column) = space[chosen[column]][row];
                    if (!field.isZero(trial(row, column))) ++weight;
                }
            }
            if (linear_algebra::rank(field, trial) == width && weight < best) best = weight;
            return;
        }
        for (std::size_t index = from; index < space.size(); ++index) {
            chosen[taken] = index;
            walk(index + 1, taken + 1);
        }
    };
    walk(0, 0);
    return best;
}

ModularMatrix matrix_of(const ModularField& field, std::size_t rows, std::size_t columns,
                        const std::vector<int64_t>& entries) {
    ModularMatrix built(rows, columns);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            field.init(built(row, column), entries[row * columns + column]);
        }
    }
    return built;
}

}  // namespace

int main() {
    const ModularField field(2);

    // Two columns of weight three whose sum has weight two. The greedy must take
    // the sum first, which is the whole of the algorithm in one case.
    const ModularMatrix pair = matrix_of(field, 4, 2, {1, 1, 1, 1, 1, 0, 0, 1});
    const ModularMatrix sparser =
        matrix_sparsification::sparsest_basis_over_a_finite_field(field, pair);
    check::equal("a pair whose sum is lighter", linear_algebra::nonzero_count(field, sparser), 5);
    check::equal("and it spans what it was given",
                 linear_algebra::same_row_space(field,
                                                linear_algebra::transpose<ModularField>(pair),
                                                linear_algebra::transpose<ModularField>(sparser)),
                 1);

    // Against brute force, on shapes small enough to enumerate every basis.
    const std::vector<std::vector<int64_t>> cases = {
        {1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1},  // 4x3
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1},  // 5x3
        {1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0},  // 6x3
    };
    const std::vector<std::size_t> rows_of = {4, 5, 6};
    for (std::size_t which = 0; which < cases.size(); ++which) {
        const ModularMatrix given = matrix_of(field, rows_of[which], 3, cases[which]);
        const ModularMatrix answer =
            matrix_sparsification::sparsest_basis_over_a_finite_field(field, given);
        const std::size_t greedy = linear_algebra::nonzero_count(field, answer);
        // A case whose columns are dependent has no basis of that size at all,
        // and brute force would silently return "none". That is a defect in the
        // case rather than in the code, so it is caught here loudly: the first
        // draft of this file had one and read as a failing algorithm.
        const std::size_t optimum = lightest_basis_by_brute_force(field, given);
        check::equal("case " + std::to_string(which) + ": the columns are independent",
                     optimum != static_cast<std::size_t>(-1), 1);
        check::equal("case " + std::to_string(which) + ": the greedy matches every basis tried",
                     greedy, static_cast<long long>(optimum));
        check::equal("case " + std::to_string(which) + ": and spans the same columns",
                     linear_algebra::same_row_space(
                         field, linear_algebra::transpose<ModularField>(given),
                         linear_algebra::transpose<ModularField>(answer)),
                     1);
        check::equal("case " + std::to_string(which) + ": and is never heavier than the input",
                     greedy <= linear_algebra::nonzero_count(field, given), 1);
    }

    return check::report("finite_field_sparsifier");
}
