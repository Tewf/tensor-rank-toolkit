#include "span_enumeration.h"

#include <stdexcept>

namespace bilinear_rank {

Matrix linear_combination(const Field& field, const std::vector<Matrix>& slices,
               const std::vector<int64_t>& coefficients) {
    if (slices.empty()) return Matrix();
    Matrix result(slices.front().rows(), slices.front().columns());
    for (std::size_t index = 0; index < slices.size(); ++index) {
        if (coefficients[index] == 0) continue;
        for (std::size_t entry = 0; entry < result.entry_count(); ++entry) {
            // r += c * s, in the field.
            field.axpyin(result.data()[entry], coefficients[index], slices[index].data()[entry]);
        }
    }
    return result;
}

std::vector<int64_t> coefficient_vector(std::size_t index, std::size_t count,
                                        int64_t characteristic) {
    std::vector<int64_t> coefficients(count);
    for (std::size_t position = 0; position < count; ++position) {
        coefficients[position] =
            static_cast<int64_t>(index % static_cast<std::size_t>(characteristic));
        index /= static_cast<std::size_t>(characteristic);
    }
    return coefficients;
}

std::size_t span_size(const Field& field, std::size_t slice_count) {
    const auto characteristic = static_cast<std::size_t>(field.characteristic());
    const std::size_t ceiling = std::size_t(1) << 40;

    std::size_t size = 1;
    for (std::size_t step = 0; step < slice_count; ++step) {
        // Tested before multiplying, so nothing above the ceiling is returned.
        // Tested after, as it was, the last step could still multiply through
        // and hand back p times more than the caller is prepared to hold, and
        // the caller reserves a vector of exactly that many candidates.
        if (size > ceiling / characteristic) {
            throw std::runtime_error("span too large to enumerate exhaustively");
        }
        size *= characteristic;
    }
    return size;
}

}  // namespace bilinear_rank
