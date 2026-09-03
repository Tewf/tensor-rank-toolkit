#include "canonical_basis.h"

namespace canonical_factorisation {

std::vector<ModularMatrix> canonical_basis(std::size_t rows, std::size_t columns) {
    std::vector<ModularMatrix> basis;
    basis.reserve(rows * columns);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            ModularMatrix element(rows, columns);
            element(row, column) = 1;
            basis.push_back(element);
        }
    }
    return basis;
}

ModularMatrix matrix_of(const ModularMatrix& coefficients, std::size_t index, std::size_t rows,
                        std::size_t columns) {
    ModularMatrix result(rows, columns);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            result(row, column) = coefficients(index, row * columns + column);
        }
    }
    return result;
}

ModularMatrix slice_matrix(const std::vector<ModularMatrix>& slices) {
    if (slices.empty()) return ModularMatrix();
    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();

    ModularMatrix stacked(slices.size(), rows * columns);
    for (std::size_t slice = 0; slice < slices.size(); ++slice) {
        for (std::size_t entry = 0; entry < rows * columns; ++entry) {
            stacked(slice, entry) = slices[slice].data()[entry];
        }
    }
    return stacked;
}

}  // namespace canonical_factorisation
