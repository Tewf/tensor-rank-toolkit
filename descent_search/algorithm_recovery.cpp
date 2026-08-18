#include "algorithm_recovery.h"

#include "span_queries.h"

namespace bilinear_rank {

namespace {

/// Index of the first row with a nonzero entry, or rows() if there is none.
std::size_t first_nonzero_row(const Field& field, const Matrix& matrix) {
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.columns(); ++column) {
            if (!field.isZero(matrix(row, column))) return row;
        }
    }
    return matrix.rows();
}

}  // namespace

bool is_scalar_multiple(const Field& field, const std::vector<Element>& from,
                     const std::vector<Element>& to, Element& scalar) {
    field.assign(scalar, field.zero);

    std::size_t pivot = from.size();
    for (std::size_t index = 0; index < from.size(); ++index) {
        if (!field.isZero(from[index])) {
            pivot = index;
            break;
        }
    }

    if (pivot == from.size()) {
        // Nothing is a multiple of the zero vector except zero itself.
        for (const Element& entry : to) {
            if (!field.isZero(entry)) return false;
        }
        return true;
    }

    Element inverse;
    field.inv(inverse, from[pivot]);
    field.mul(scalar, to[pivot], inverse);

    for (std::size_t index = 0; index < from.size(); ++index) {
        Element expected;
        field.mul(expected, from[index], scalar);
        // Asked of the field, not of the machine integer holding it: two
        // representatives of one residue are the same scalar.
        if (!field.areEqual(expected, to[index])) return false;
    }
    return true;
}

std::vector<Matrix> encoded_products(const Field& field, const Matrix& left,
                                     const Matrix& right) {
    std::vector<Matrix> products;
    products.reserve(left.rows());
    for (std::size_t product = 0; product < left.rows(); ++product) {
        Matrix outer(left.columns(), right.columns());
        for (std::size_t row = 0; row < left.columns(); ++row) {
            for (std::size_t column = 0; column < right.columns(); ++column) {
                field.mul(outer(row, column), left(product, row), right(product, column));
            }
        }
        products.push_back(std::move(outer));
    }
    return products;
}

bool recover_operands(const Field& field, const std::vector<Matrix>& products, Matrix& left,
                      Matrix& right) {
    if (products.empty()) return false;
    const std::size_t rows = products.front().rows();
    const std::size_t columns = products.front().columns();

    left = Matrix(products.size(), rows);
    right = Matrix(products.size(), columns);

    for (std::size_t product = 0; product < products.size(); ++product) {
        const Matrix& slice = products[product];
        const std::size_t leading = first_nonzero_row(field, slice);
        if (leading == rows) return false;  // the zero map is not a product

        // Every row of a rank-one matrix is a multiple of any nonzero row, so
        // the leading one is the right-hand operand and the multipliers are the
        // left-hand one. Taking the leading row and not row 0 is the fix.
        const std::vector<Element> leading_row = slice.row(leading);
        for (std::size_t column = 0; column < columns; ++column) {
            right(product, column) = leading_row[column];
        }

        field.assign(left(product, leading), field.one);
        for (std::size_t row = leading + 1; row < rows; ++row) {
            Element multiplier;
            if (!is_scalar_multiple(field, leading_row, slice.row(row), multiplier)) {
                return false;  // not rank one
            }
            left(product, row) = multiplier;
        }
    }
    return true;
}

bool recover_decoder(const Field& field, const std::vector<Matrix>& target,
                     const std::vector<Matrix>& products, Matrix& decode) {
    if (products.empty()) return target.empty();

    std::vector<std::vector<Element>> flattened;
    flattened.reserve(products.size());
    for (const Matrix& product : products) {
        flattened.push_back(linear_algebra::SpanBasis<Field>::flatten(product));
    }

    decode = Matrix(target.size(), products.size());
    for (std::size_t output = 0; output < target.size(); ++output) {
        std::vector<Element> coefficients;
        if (!linear_algebra::solve_in_row_space(
                field, flattened, linear_algebra::SpanBasis<Field>::flatten(target[output]),
                coefficients)) {
            return false;  // this output is not reachable from these products
        }
        for (std::size_t product = 0; product < products.size(); ++product) {
            decode(output, product) = coefficients[product];
        }
    }
    return true;
}

bool recover_algorithm(const Field& field, const std::vector<Matrix>& target,
                       const std::vector<Matrix>& products, Algorithm& algorithm) {
    return recover_operands(field, products, algorithm.left, algorithm.right) &&
           recover_decoder(field, target, products, algorithm.decode);
}

std::vector<Matrix> map_computed_by(const Field& field, const Algorithm& algorithm) {
    const std::vector<Matrix> products =
        encoded_products(field, algorithm.left, algorithm.right);

    std::vector<Matrix> outputs;
    outputs.reserve(algorithm.decode.rows());
    for (std::size_t output = 0; output < algorithm.decode.rows(); ++output) {
        Matrix slice(algorithm.left.columns(), algorithm.right.columns());
        for (std::size_t product = 0; product < products.size(); ++product) {
            const Element weight = algorithm.decode(output, product);
            if (field.isZero(weight)) continue;
            for (std::size_t entry = 0; entry < slice.entry_count(); ++entry) {
                field.axpyin(slice.data()[entry], weight, products[product].data()[entry]);
            }
        }
        outputs.push_back(std::move(slice));
    }
    return outputs;
}

bool recovers_map(const Field& field, const std::vector<Matrix>& target,
                  const std::vector<Matrix>& products, Algorithm& algorithm) {
    if (!recover_algorithm(field, target, products, algorithm)) return false;
    return linear_algebra::spans_all(field, map_computed_by(field, algorithm), target);
}

}  // namespace bilinear_rank
