/// The determinant test, against the worksheet it comes from.
///
/// Every number here is read off `cex_poldet`'s own output rather than off this
/// implementation, which is the only way the test is worth running: the
/// worksheet's B has 12 nonzeros in 28 entries, so 16 of them are zero whatever
/// the unknowns are.
#include <iostream>
#include <string>
#include <vector>

#include "check.h"
#include "dense_matrix_file.h"
#include "measures.h"
#include "multivariate_polynomial.h"
#include "pattern_feasibility.h"

namespace {

using matrix_sparsification::Field;
using matrix_sparsification::Matrix;
using matrix_sparsification::Polynomial;

/// The worksheet's own choice: one unknown in each pinned row, in a different
/// column each time. Pinned rows are 1, 7, 3, 4 of the operator, so the block's
/// rows carry a_11, a_74, a_32 and a_43 in that order.
std::vector<std::vector<bool>> worksheet_choice() {
    return {{true, false, false, false},
            {false, false, false, true},
            {false, true, false, false},
            {false, false, true, false}};
}

const std::vector<std::size_t> kPinnedRows = {0, 6, 2, 3};

void check_polynomial_arithmetic(const Field& field) {
    // (x0 - x1)(x0 + x1) = x0^2 - x1^2: three terms before collection, two
    // after, and the cross terms must actually cancel rather than linger with a
    // zero coefficient.
    Polynomial left = Polynomial::variable(field, 0);
    left.subtract_in(field, Polynomial::variable(field, 1));
    Polynomial right = Polynomial::variable(field, 0);
    right.add_in(field, Polynomial::variable(field, 1));

    const Polynomial product = left.multiplied_by(field, right);
    check::equal("(x0-x1)(x0+x1) term count", static_cast<long long>(product.term_count()), 2);
    check::equal("(x0-x1)(x0+x1) degree", static_cast<long long>(product.degree()), 2);

    // x0 - x0 is zero, and has to be empty rather than a stored zero.
    Polynomial cancelling = Polynomial::variable(field, 0);
    cancelling.subtract_in(field, Polynomial::variable(field, 0));
    check::equal("x0-x0 is zero", cancelling.is_zero() ? 1 : 0, 1);
}

void check_worksheet(const Field& field, const Matrix& operator_) {
    check::equal("worksheet operator nonzeros",
                 static_cast<long long>(linear_algebra::nonzero_count(field, operator_)), 14);

    const auto verdict =
        matrix_sparsification::decide_pattern(field, operator_, kPinnedRows, worksheet_choice());

    check::equal("worksheet choice is realisable", verdict.realisable ? 1 : 0, 1);
    check::equal("obstruction degree", static_cast<long long>(verdict.obstruction.degree()), 4);
    check::equal("entries zero whatever the unknowns",
                 static_cast<long long>(verdict.guaranteed_zeros), 16);
    check::equal("a witness was found", verdict.witnessed ? 1 : 0, 1);
    check::equal("nonzeros the worksheet reaches",
                 static_cast<long long>(linear_algebra::nonzero_count(field, verdict.result)), 12);
}

/// Two patterns that cannot be realised, for opposite reasons.
void check_refusals(const Field& field, const Matrix& operator_) {
    const std::vector<std::vector<bool>> nothing_free(4, std::vector<bool>(4, false));
    const auto empty =
        matrix_sparsification::decide_pattern(field, operator_, kPinnedRows, nothing_free);
    check::equal("an all-zero target is refused", empty.realisable ? 1 : 0, 0);

    // Both unknowns in one column: the target has rank at most one, so the
    // change of basis is singular for every assignment.
    std::vector<std::vector<bool>> one_column(4, std::vector<bool>(4, false));
    one_column[0][0] = true;
    one_column[1][0] = true;
    const auto degenerate =
        matrix_sparsification::decide_pattern(field, operator_, kPinnedRows, one_column);
    check::equal("a rank-deficient target is refused", degenerate.realisable ? 1 : 0, 0);
}

void check_singular_pinning(const Field& field, const Matrix& operator_) {
    // Row 1 of this operator is row 2 plus row 4, so any pinning containing all
    // three fixes no basis and the call must say so rather than invert
    // something singular.
    bool threw = false;
    try {
        matrix_sparsification::decide_pattern(field, operator_, {0, 1, 3, 2}, worksheet_choice());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check::equal("singular pinning is refused", threw ? 1 : 0, 1);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    const Field field;
    const Matrix operator_ =
        formats::read_rational_matrix_file(fixtures + "/dumas_counterexample_l.matrix");

    check_polynomial_arithmetic(field);
    check_worksheet(field, operator_);
    check_refusals(field, operator_);
    check_singular_pinning(field, operator_);
    return check::report("pattern feasibility");
}
