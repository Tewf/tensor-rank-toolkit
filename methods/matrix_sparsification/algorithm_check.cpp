#include "algorithm_check.h"

namespace matrix_sparsification {

namespace {

bool has_shape(const Matrix& matrix, std::size_t rows, std::size_t columns) {
    return matrix.rows() == rows && matrix.columns() == columns;
}

}  // namespace

bool computes_matrix_product(const Field& field, const BilinearAlgorithm& algorithm,
                             std::size_t rows, std::size_t inner, std::size_t columns) {
    const std::size_t products = algorithm.left_encoding.rows();
    if (!has_shape(algorithm.left_encoding, products, rows * inner)) return false;
    if (!has_shape(algorithm.right_encoding, products, inner * columns)) return false;
    if (!has_shape(algorithm.decoding, products, rows * columns)) return false;

    for (std::size_t output = 0; output < rows * columns; ++output) {
        const std::size_t result_row = output / columns;
        const std::size_t result_column = output % columns;

        for (std::size_t left = 0; left < rows * inner; ++left) {
            for (std::size_t right = 0; right < inner * columns; ++right) {
                Element total;
                field.assign(total, field.zero);
                for (std::size_t product = 0; product < products; ++product) {
                    Element term;
                    field.mul(term, algorithm.left_encoding(product, left),
                              algorithm.right_encoding(product, right));
                    field.mulin(term, algorithm.decoding(product, output));
                    field.addin(total, term);
                }

                // a_ij b_jl contributes to c_il exactly when the row of a, the
                // column of b and the shared index all line up.
                const bool contributes = left / inner == result_row &&
                                         right % columns == result_column &&
                                         left % inner == right / columns;
                const bool correct = contributes ? field.isOne(total) : field.isZero(total);
                if (!correct) return false;
            }
        }
    }
    return true;
}

}  // namespace matrix_sparsification
