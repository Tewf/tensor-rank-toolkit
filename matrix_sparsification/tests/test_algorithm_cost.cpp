/// What Strassen costs, and that the operators saying so are really Strassen.
///
/// The numbers checked here are the ones everybody knows, which is the point:
/// 7*N^log2(7) - 6*N^2 for Strassen and 6*N^log2(7) - 5*N^2 for Winograd. If
/// this file agrees with them, Claim 2.11 and Claim 3.9 are implemented right.
#include <string>

#include "algorithm_check.h"
#include "algorithm_cost.h"
#include "check.h"
#include "dense_matrix_file.h"
#include "greedy_sparsifier.h"
#include "matrix_ops.h"
#include "measures.h"

namespace {

using matrix_sparsification::BilinearAlgorithm;
using matrix_sparsification::Field;
using matrix_sparsification::Matrix;

/// Report an exact rational as 1 when it equals `expected` and 0 otherwise,
/// so `check::equal` can print it without converting a Givaro integer.
long long matches(const Field& field, const matrix_sparsification::Element& value,
                  long long expected) {
    matrix_sparsification::Element wanted;
    field.init(wanted, static_cast<int64_t>(expected));
    return field.areEqual(value, wanted) ? 1 : 0;
}

/// Sparsify one operator with the best of what this folder has, on the side it
/// is stored transposed on.
Matrix sparsified(const Field& field, const Matrix& operator_) {
    return linear_algebra::transpose<Field>(
        matrix_sparsification::sparsify_by_rescaling(field, linear_algebra::transpose<Field>(operator_)));
}

}  // namespace

int main(int argc, char** argv) {
    const std::string fixtures = argc > 1 ? argv[1] : "fixtures";
    const Field field;

    BilinearAlgorithm strassen;
    strassen.left_encoding =
        linear_algebra::read_rational_matrix_file(fixtures + "/strassen_u.matrix");
    strassen.right_encoding =
        linear_algebra::read_rational_matrix_file(fixtures + "/strassen_v.matrix");
    strassen.decoding = linear_algebra::read_rational_matrix_file(fixtures + "/strassen_w.matrix");

    check::equal("the three operators are Strassen's algorithm",
                 matrix_sparsification::computes_matrix_product(field, strassen, 2, 2, 2) ? 1 : 0,
                 1);

    const std::size_t left = matrix_sparsification::encoding_complexity(field, strassen.left_encoding);
    const std::size_t right =
        matrix_sparsification::encoding_complexity(field, strassen.right_encoding);
    const std::size_t out = matrix_sparsification::decoding_complexity(field, strassen.decoding);

    check::equal("q_u", static_cast<long long>(left), 5);
    check::equal("q_v", static_cast<long long>(right), 5);
    check::equal("q_w", static_cast<long long>(out), 8);
    check::equal("Strassen's 18 additions", static_cast<long long>(left + right + out), 18);
    check::equal("Strassen's leading coefficient is 7",
                 matches(field,
                         matrix_sparsification::leading_coefficient(field, left, right, out, 7, 2,
                                                                    2, 2),
                         7),
                 1);

    // Winograd's variant is the same algorithm with 15 additions rather than
    // 18, and the formula has to turn that into 6 rather than 7.
    check::equal("Winograd's leading coefficient is 6",
                 matches(field,
                         matrix_sparsification::leading_coefficient(field, 4, 4, 7, 7, 2, 2, 2), 6),
                 1);

    // What this folder's own sparsification buys, end to end: the operators get
    // sparser, so the additive complexity drops, so the leading coefficient
    // drops. This is the whole argument for the strand in one assertion.
    BilinearAlgorithm sparser;
    sparser.left_encoding = sparsified(field, strassen.left_encoding);
    sparser.right_encoding = sparsified(field, strassen.right_encoding);
    sparser.decoding = sparsified(field, strassen.decoding);

    const std::size_t sparse_left =
        matrix_sparsification::encoding_complexity(field, sparser.left_encoding);
    const std::size_t sparse_right =
        matrix_sparsification::encoding_complexity(field, sparser.right_encoding);
    const std::size_t sparse_out =
        matrix_sparsification::decoding_complexity(field, sparser.decoding);

    check::equal("additions after sparsifying",
                 static_cast<long long>(sparse_left + sparse_right + sparse_out), 12);
    check::equal("leading coefficient after sparsifying is 5",
                 matches(field,
                         matrix_sparsification::leading_coefficient(
                             field, sparse_left, sparse_right, sparse_out, 7, 2, 2, 2),
                         5),
                 1);

    // A shape that cannot be an algorithm has to be refused rather than read
    // out of range.
    BilinearAlgorithm mismatched = strassen;
    mismatched.decoding = strassen.left_encoding;
    check::equal("wrong shapes are refused",
                 matrix_sparsification::computes_matrix_product(field, mismatched, 2, 2, 3) ? 1 : 0,
                 0);

    return check::report("algorithm cost");
}
