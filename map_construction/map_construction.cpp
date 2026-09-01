#include "map_construction.h"

#include <givaro/givpoly1.h>
#include <givaro/givpoly1factor.h>

#include <stdexcept>
#include <string>

#include "memory_budget.h"

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

    // `rows*columns` slices of `rows*inner` by `inner*columns`, which is
    // `(rows*inner*columns)^2` entries: cubic in each dimension and quadratic in
    // the whole, from three numbers `make-tensor --matmul` takes without a
    // ceiling. `--matmul 2 100 100 100` is 10^12 entries, and a caller who asked
    // for it got a kill rather than the number.
    //
    // Asked as one slice and then as a count of slices, rather than as one
    // product: `left_width * right_width` is itself a multiplication that can
    // wrap on the input this exists to refuse, and one slice inside the budget
    // makes `bytes_per_matrix` safe by construction.
    const std::string product = std::to_string(rows) + "x" + std::to_string(inner) + "x" +
                                std::to_string(columns) + " matrix product";
    run_limits::require_room("one slice of a " + product, left_width, right_width * sizeof(Element));
    run_limits::require_room("the slices of a " + product, rows * columns,
                 run_limits::bytes_per_matrix(left_width * right_width));

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
    // `length` slices of `length` by `length`, so cubic in the one number
    // `--cyclic` takes: 1.25e11 entries at `--cyclic 2 5000`. One slice, then the
    // count of them, for the reason the matrix product above gives.
    const std::string convolution = "a cyclic convolution of length " + std::to_string(length);
    run_limits::require_room("one slice of " + convolution, length, length * sizeof(Element));
    run_limits::require_room("the slices of " + convolution, length, run_limits::bytes_per_matrix(length * length));

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
