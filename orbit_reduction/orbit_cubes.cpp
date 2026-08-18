#include "orbit_cubes.h"

#include <stdexcept>
#include <string>

#include "map_construction.h"
#include "pool_orbits.h"

namespace bilinear_rank {

namespace {

std::string shape_of(std::size_t rows, std::size_t inner, std::size_t columns) {
    return "<" + std::to_string(rows) + "," + std::to_string(inner) + "," +
           std::to_string(columns) + ">";
}

/// Whether `slices` is the matrix multiplication tensor of that shape, entry for
/// entry. Cheap next to anything a solver then does with it.
bool is_matrix_multiplication(const Field& field, const std::vector<Matrix>& slices,
                              std::size_t rows, std::size_t inner, std::size_t columns) {
    const std::vector<Matrix> wanted = matrix_multiplication_tensor(rows, inner, columns);
    if (slices.size() != wanted.size()) return false;
    for (std::size_t slice = 0; slice < slices.size(); ++slice) {
        if (slices[slice].rows() != wanted[slice].rows()) return false;
        if (slices[slice].columns() != wanted[slice].columns()) return false;
        for (std::size_t entry = 0; entry < wanted[slice].entry_count(); ++entry) {
            if (!field.areEqual(slices[slice].data()[entry], wanted[slice].data()[entry])) {
                return false;
            }
        }
    }
    return true;
}

/// The variables of term zero, from an array holding every term's.
///
/// Refuses a length that is not a whole number of terms, which is the way a
/// consumer using a different layout announces itself before the literals go out
/// wrong rather than after.
std::vector<int> first_term(const std::vector<int>& variables, std::size_t width,
                            const char* which) {
    if (width == 0) throw std::runtime_error("a term with no coordinates cannot be pinned");
    if (variables.size() < width || variables.size() % width != 0) {
        throw std::runtime_error(std::string("the ") + which + " variables number " +
                                 std::to_string(variables.size()) + ", which is not a whole number "
                                 "of terms of " + std::to_string(width) + " coordinates");
    }
    return std::vector<int>(variables.begin(), variables.begin() + static_cast<long>(width));
}

}  // namespace

std::vector<std::vector<int>> orbit_cubes(const Field& field, const std::vector<Matrix>& slices,
                                          std::size_t rows, std::size_t inner, std::size_t columns,
                                          const std::vector<int>& left_variables,
                                          const std::vector<int>& right_variables) {
    if (field.characteristic() != 2) {
        // Over larger fields a rank-one map is a whole scalar class of operand
        // pairs, so pinning one pair would forbid the others and refuse
        // solutions that exist. The representatives are still right; only this
        // way of writing them down is not.
        throw std::runtime_error("orbit cubes are written for GF(2)");
    }
    if (!is_matrix_multiplication(field, slices, rows, inner, columns)) {
        throw std::runtime_error("these representatives are the orbits of " +
                                 shape_of(rows, inner, columns) +
                                 ", and the tensor given is not that map");
    }

    const std::vector<int> left = first_term(left_variables, rows * inner, "left");
    const std::vector<int> right = first_term(right_variables, inner * columns, "right");

    std::vector<std::vector<int>> cubes;
    for (const auto& [operand, coperand] :
         matrix_multiplication_orbit_vectors(field, rows, inner, columns)) {
        std::vector<int> cube;
        cube.reserve(operand.size() + coperand.size());
        for (std::size_t index = 0; index < operand.size(); ++index) {
            cube.push_back(field.isZero(operand[index]) ? -left[index] : left[index]);
        }
        for (std::size_t index = 0; index < coperand.size(); ++index) {
            cube.push_back(field.isZero(coperand[index]) ? -right[index] : right[index]);
        }
        cubes.push_back(std::move(cube));
    }
    return cubes;
}

}  // namespace bilinear_rank
