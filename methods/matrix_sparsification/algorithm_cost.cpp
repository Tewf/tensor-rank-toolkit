#include "algorithm_cost.h"

#include <stdexcept>
#include <string>

#include "measures.h"

namespace matrix_sparsification {

std::size_t encoding_complexity(const Field& field, const Matrix& operator_) {
    const std::size_t operations = linear_algebra::operation_count(field, operator_);
    return operations < operator_.rows() ? 0 : operations - operator_.rows();
}

std::size_t decoding_complexity(const Field& field, const Matrix& operator_) {
    const std::size_t operations = linear_algebra::operation_count(field, operator_);
    return operations < operator_.columns() ? 0 : operations - operator_.columns();
}

namespace {

/// `numerator / (products - block)`, refusing the degenerate case rather than
/// dividing by zero.
///
/// `products == block` means the algorithm uses exactly as many products as the
/// naive one, so there is nothing recursive to amortise and Claim 3.9's
/// geometric sum does not converge to the expression it gives.
Element ratio(const Field& field, std::size_t numerator, std::size_t products, std::size_t block,
              const char* which) {
    if (products <= block) {
        throw std::invalid_argument(std::string("a recursive algorithm needs more products than "
                                                "the naive one, but ") +
                                    which + " has " + std::to_string(block) + " of " +
                                    std::to_string(products));
    }
    Element value;
    field.init(value, static_cast<int64_t>(numerator), static_cast<int64_t>(products - block));
    return value;
}

}  // namespace

Element leading_coefficient(const Field& field, std::size_t encoding_left,
                            std::size_t encoding_right, std::size_t decoding, std::size_t products,
                            std::size_t rows, std::size_t inner, std::size_t columns) {
    Element total = ratio(field, encoding_left, products, rows * inner, "the left operand");
    field.addin(total, ratio(field, encoding_right, products, inner * columns, "the right operand"));
    field.addin(total, ratio(field, decoding, products, rows * columns, "the result"));
    field.addin(total, field.one);
    return total;
}

}  // namespace matrix_sparsification
