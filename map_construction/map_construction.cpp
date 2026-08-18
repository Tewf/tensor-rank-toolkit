#include "map_construction.h"

#include <givaro/givpoly1.h>
#include <givaro/givpoly1factor.h>

#include <stdexcept>

namespace bilinear_rank {

namespace {

bool is_zero_matrix(const Field& field, const Matrix& matrix) {
    for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
        if (!field.isZero(matrix.data()[entry])) return false;
    }
    return true;
}

}  // namespace

std::vector<Matrix> polynomial_multiplication_tensor(std::size_t left_terms,
                                                     std::size_t right_terms) {
    std::vector<Matrix> slices;
    slices.reserve(left_terms + right_terms - 1);
    for (std::size_t output = 0; output < left_terms + right_terms - 1; ++output) {
        Matrix slice(left_terms, right_terms);
        for (std::size_t left = 0; left < left_terms; ++left) {
            for (std::size_t right = 0; right < right_terms; ++right) {
                if (left + right == output) slice(left, right) = 1;
            }
        }
        slices.push_back(std::move(slice));
    }
    return slices;
}

bool is_irreducible(const Field& field, const Polynomial& modulus) {
    if (modulus.size() < 2) return false;

    Givaro::Poly1FactorDom<Field, Givaro::Dense> polynomials(field);
    // Givaro stores the lowest degree first; the fixtures write the highest
    // first for readability.
    Givaro::Poly1FactorDom<Field, Givaro::Dense>::Element ascending(modulus.rbegin(),
                                                                   modulus.rend());
    return polynomials.is_irreducible(ascending);
}

std::vector<Matrix> reduce_tensor_modulo(const Field& field, std::vector<Matrix> slices,
                                         const Polynomial& modulus) {
    if (modulus.empty()) throw std::runtime_error("cannot reduce by an empty modulus");
    if (field.isZero(modulus.front())) {
        throw std::runtime_error("the modulus must have a nonzero leading coefficient");
    }
    if (slices.size() < modulus.size()) return slices;  // already of lower degree

    Element leading_inverse;
    field.inv(leading_inverse, modulus.front());
    const std::size_t last_start = slices.size() - modulus.size();

    for (std::size_t start = 0; start <= last_start; ++start) {
        if (is_zero_matrix(field, slices[start])) continue;

        // The quotient coefficient is a matrix: slices[start] / modulus[0].
        Matrix quotient = slices[start];
        for (std::size_t entry = 0; entry < quotient.entry_count(); ++entry) {
            field.mulin(quotient.data()[entry], leading_inverse);
        }
        for (std::size_t step = 0; step < modulus.size(); ++step) {
            Element weight;
            field.neg(weight, modulus[step]);
            for (std::size_t entry = 0; entry < quotient.entry_count(); ++entry) {
                field.axpyin(slices[start + step].data()[entry], weight, quotient.data()[entry]);
            }
        }
    }

    // The remainder is what is left below the modulus's degree, with zero
    // coefficients dropped to keep the tensor compact.
    std::vector<Matrix> remainder;
    for (Matrix& slice : slices) {
        if (!is_zero_matrix(field, slice)) remainder.push_back(std::move(slice));
    }
    return remainder;
}

std::vector<Matrix> field_multiplication_tensor(const Field& field, const Polynomial& modulus) {
    if (!is_irreducible(field, modulus)) {
        throw std::runtime_error("the modulus is not irreducible, so this is not a field");
    }
    const std::size_t degree = modulus.size() - 1;

    // Elements of GF(p^degree) have `degree` coefficients.
    std::vector<Matrix> product = polynomial_multiplication_tensor(degree, degree);
    std::vector<Matrix> descending(product.rbegin(), product.rend());
    return reduce_tensor_modulo(field, std::move(descending), modulus);
}

std::vector<Matrix> matrix_multiplication_tensor(std::size_t rows, std::size_t inner,
                                                 std::size_t columns) {
    const std::size_t left_width = rows * inner;      // A, read row by row
    const std::size_t right_width = inner * columns;  // B, read row by row

    std::vector<Matrix> slices;
    slices.reserve(rows * columns);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            Matrix slice(left_width, right_width);
            // C[row][column] = sum over the shared index of A[row][j]*B[j][column].
            for (std::size_t shared = 0; shared < inner; ++shared) {
                slice(row * inner + shared, shared * columns + column) = 1;
            }
            slices.push_back(std::move(slice));
        }
    }
    return slices;
}

std::vector<Matrix> cyclic_convolution_tensor(std::size_t length) {
    std::vector<Matrix> slices;
    slices.reserve(length);
    for (std::size_t output = 0; output < length; ++output) {
        Matrix slice(length, length);
        for (std::size_t left = 0; left < length; ++left) {
            for (std::size_t right = 0; right < length; ++right) {
                if ((left + right) % length == output) slice(left, right) = 1;
            }
        }
        slices.push_back(std::move(slice));
    }
    return slices;
}

}  // namespace bilinear_rank
