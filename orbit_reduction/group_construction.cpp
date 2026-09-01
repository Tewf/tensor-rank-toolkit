#include "group_construction.h"

#include <sstream>
#include <stdexcept>

#include <stdexcept>

#include "measures.h"
#include "memory_budget.h"
#include "solver.h"

namespace bilinear_rank {

namespace {

/// `(A ⊗ B)[(i,j), (a,b)] = A[i][a] · B[j][b]`, the map that turns
/// `M ↦ A M Bᵀ` into a matrix acting on `M` read row by row.
Matrix kronecker(const Field& field, const Matrix& left, const Matrix& right) {
    Matrix result(left.rows() * right.rows(), left.columns() * right.columns());
    for (std::size_t outer_row = 0; outer_row < left.rows(); ++outer_row) {
        for (std::size_t outer_column = 0; outer_column < left.columns(); ++outer_column) {
            if (field.isZero(left(outer_row, outer_column))) continue;
            for (std::size_t inner_row = 0; inner_row < right.rows(); ++inner_row) {
                for (std::size_t inner_column = 0; inner_column < right.columns(); ++inner_column) {
                    field.mul(result(outer_row * right.rows() + inner_row,
                                     outer_column * right.columns() + inner_column),
                              left(outer_row, outer_column), right(inner_row, inner_column));
                }
            }
        }
    }
    return result;
}

/// The matrix at `index`, counting in base `p` through every entry.
Matrix matrix_at(const Field& field, std::size_t order, std::size_t index) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    Matrix result(order, order);
    for (std::size_t entry = 0; entry < result.entry_count(); ++entry) {
        result.data()[entry] = static_cast<Element>(index % characteristic);
        index /= characteristic;
    }
    return result;
}

/// How many matrices of that order there are, refusing a walk too long to make.
std::size_t matrices_of_order(const Field& field, std::size_t order) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::size_t ceiling = 1u << 20;

    std::size_t count = 1;
    for (std::size_t entry = 0; entry < order * order; ++entry) {
        if (count > ceiling / characteristic) {
            throw std::runtime_error("enumerating every " + std::to_string(order) + "x" +
                                     std::to_string(order) + " matrix over GF(" +
                                     std::to_string(characteristic) +
                                     ") is more than a million of them; the group has to come from "
                                     "a closed form at this size");
        }
        count *= characteristic;
    }
    return count;
}

/// `(Y⁻¹)ᵀ`, the half of the action that undoes a change of coordinates on the
/// shared index.
Matrix inverse_transpose(const Field& field, const Matrix& square) {
    Matrix inverse;
    if (!linear_algebra::invert(field, square, inverse)) {
        throw std::runtime_error("a matrix listed as invertible was not");
    }
    return linear_algebra::transpose<Field>(inverse);
}

}  // namespace

std::vector<Matrix> general_linear_generators(const Field& field, std::size_t order) {
    std::vector<Matrix> generators;
    if (order == 0) return generators;

    // The cycle: e_i -> e_{i+1}, and a transvection adding one coordinate to
    // another. Together they generate GL over GF(2); over GF(p) a scaling of one
    // coordinate by a generator of the multiplicative group joins them.
    Matrix cycle(order, order);
    for (std::size_t index = 0; index < order; ++index) {
        field.assign(cycle(index, (index + 1) % order), field.one);
    }
    generators.push_back(std::move(cycle));

    if (order >= 2) {
        Matrix transvection(order, order);
        for (std::size_t index = 0; index < order; ++index) {
            field.assign(transvection(index, index), field.one);
        }
        field.assign(transvection(0, 1), field.one);
        generators.push_back(std::move(transvection));
    }

    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    if (characteristic > 2) {
        Matrix scaling(order, order);
        for (std::size_t index = 0; index < order; ++index) {
            field.assign(scaling(index, index), field.one);
        }
        // Any non-identity unit will do for the small primes in play here.
        scaling(0, 0) = static_cast<Element>(characteristic - 1);
        generators.push_back(std::move(scaling));
    }
    return generators;
}

namespace {

/// The automorphism a triple gives, shared by both routes below.
Automorphism sandwich(const Field& field, const Matrix& x, const Matrix& y, const Matrix& z) {
    Matrix y_inverse_transpose, z_inverse_transpose;
    Matrix inverse;
    if (!linear_algebra::invert(field, y, inverse)) throw std::runtime_error("y is not invertible");
    y_inverse_transpose = linear_algebra::transpose<Field>(inverse);
    if (!linear_algebra::invert(field, z, inverse)) throw std::runtime_error("z is not invertible");
    z_inverse_transpose = linear_algebra::transpose<Field>(inverse);
    return {kronecker(field, x, y_inverse_transpose), kronecker(field, y, z_inverse_transpose)};
}

/// The identity of that order.
Matrix unit(const Field& field, std::size_t order) {
    Matrix result(order, order);
    for (std::size_t index = 0; index < order; ++index) {
        field.assign(result(index, index), field.one);
    }
    return result;
}

}  // namespace

void require_matmul_shape(const std::vector<Matrix>& slices, std::size_t rows,
                          std::size_t inner, std::size_t columns) {
    if (slices.empty()) return;
    const std::size_t wanted_rows = rows * inner;
    const std::size_t wanted_columns = inner * columns;
    const std::size_t wanted_slices = rows * columns;
    if (slices.front().rows() == wanted_rows && slices.front().columns() == wanted_columns &&
        slices.size() == wanted_slices) {
        return;
    }

    std::ostringstream complaint;
    complaint << "--symmetry matmul " << rows << " " << inner << " " << columns << " describes "
              << wanted_slices << " slices of " << wanted_rows << "x" << wanted_columns
              << ", and the tensor given has " << slices.size() << " slices of "
              << slices.front().rows() << "x" << slices.front().columns()
              << "; a group built for one shape says nothing about another";
    throw std::runtime_error(complaint.str());
}

std::vector<Automorphism> matrix_multiplication_symmetry_generators(const Field& field,
                                                                    std::size_t rows,
                                                                    std::size_t inner,
                                                                    std::size_t columns) {
    // One generator at a time in one of the three factors, the other two left
    // alone: that generates the same group as all triples.
    std::vector<Automorphism> generators;
    for (const Matrix& x : general_linear_generators(field, rows)) {
        generators.push_back(sandwich(field, x, unit(field, inner), unit(field, columns)));
    }
    for (const Matrix& y : general_linear_generators(field, inner)) {
        generators.push_back(sandwich(field, unit(field, rows), y, unit(field, columns)));
    }
    for (const Matrix& z : general_linear_generators(field, columns)) {
        generators.push_back(sandwich(field, unit(field, rows), unit(field, inner), z));
    }
    return generators;
}

std::vector<Matrix> general_linear_group(const Field& field, std::size_t order) {
    const std::size_t candidates = matrices_of_order(field, order);

    std::vector<Matrix> invertible;
    for (std::size_t index = 0; index < candidates; ++index) {
        Matrix candidate = matrix_at(field, order, index);
        if (linear_algebra::rank(field, candidate) == order) invertible.push_back(std::move(candidate));
    }
    return invertible;
}

std::vector<Automorphism> all_automorphisms(const Field& field, std::size_t rows,
                                            std::size_t columns) {
    const std::vector<Matrix> lefts = general_linear_group(field, rows);
    const std::vector<Matrix> rights = general_linear_group(field, columns);

    run_limits::require_room("every automorphism of that shape", lefts.size() * rights.size(),
                 run_limits::bytes_per_matrix(rows * rows) + run_limits::bytes_per_matrix(columns * columns));

    std::vector<Automorphism> group;
    group.reserve(lefts.size() * rights.size());
    for (const Matrix& left : lefts) {
        for (const Matrix& right : rights) group.push_back({left, right});
    }
    return group;
}

std::vector<Automorphism> matrix_multiplication_symmetries(const Field& field, std::size_t rows,
                                                           std::size_t inner,
                                                           std::size_t columns) {
    const std::vector<Matrix> on_rows = general_linear_group(field, rows);
    const std::vector<Matrix> on_inner = general_linear_group(field, inner);
    const std::vector<Matrix> on_columns = general_linear_group(field, columns);

    const std::size_t count = on_rows.size() * on_inner.size() * on_columns.size();
    run_limits::require_room("the symmetries of that matrix product", count,
                 run_limits::bytes_per_matrix(rows * inner * rows * inner) +
                     run_limits::bytes_per_matrix(inner * columns * inner * columns));

    std::vector<Automorphism> group;
    group.reserve(count);
    for (const Matrix& x : on_rows) {
        for (const Matrix& y : on_inner) {
            const Matrix y_inverse_transpose = inverse_transpose(field, y);
            for (const Matrix& z : on_columns) {
                group.push_back({kronecker(field, x, y_inverse_transpose),
                                 kronecker(field, y, inverse_transpose(field, z))});
            }
        }
    }
    return group;
}

}  // namespace bilinear_rank
