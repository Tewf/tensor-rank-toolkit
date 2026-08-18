#include "projection_lower_bound.h"

#include "kronecker_structure.h"
#include "measures.h"
#include "span_basis.h"

namespace pencil_rank {

namespace {

/// Every point of the projective space over `GF(p)^length`, as leading-one
/// vectors. One representative per scalar class, because a plane depends on the
/// classes and not on the vectors.
std::vector<std::vector<int64_t>> projective_points(const ModularField& field,
                                                    std::size_t length) {
    const std::size_t characteristic = static_cast<std::size_t>(field.residu());
    std::vector<std::vector<int64_t>> points;

    for (std::size_t leading = 0; leading < length; ++leading) {
        std::size_t free_positions = length - leading - 1;
        std::size_t count = 1;
        for (std::size_t step = 0; step < free_positions; ++step) count *= characteristic;

        for (std::size_t pattern = 0; pattern < count; ++pattern) {
            std::vector<int64_t> point(length, 0);
            point[leading] = 1;
            std::size_t rest = pattern;
            for (std::size_t position = leading + 1; position < length; ++position) {
                field.init(point[position], static_cast<int64_t>(rest % characteristic));
                rest /= characteristic;
            }
            points.push_back(point);
        }
    }
    return points;
}

/// `sum_j weight[j] * slices[j]`, which is the slice axis projected onto one
/// direction.
ModularMatrix combined(const ModularField& field, const std::vector<ModularMatrix>& slices,
                       const std::vector<int64_t>& weight) {
    ModularMatrix result(slices.front().rows(), slices.front().columns());
    for (std::size_t index = 0; index < slices.size(); ++index) {
        if (field.isZero(weight[index])) continue;
        for (std::size_t entry = 0; entry < result.entry_count(); ++entry) {
            field.axpyin(result.data()[entry], weight[index], slices[index].data()[entry]);
        }
    }
    return result;
}

/// The `C(n, 2)` coordinates of `left ^ right`, laid out as one row.
std::vector<int64_t> wedge(const ModularField& field, const std::vector<int64_t>& left,
                           const std::vector<int64_t>& right) {
    std::vector<int64_t> row;
    row.reserve(left.size() * (left.size() - 1) / 2);
    for (std::size_t i = 0; i < left.size(); ++i) {
        for (std::size_t j = i + 1; j < left.size(); ++j) {
            int64_t forward = 0;
            int64_t backward = 0;
            field.mul(forward, left[i], right[j]);
            field.mul(backward, left[j], right[i]);
            field.subin(forward, backward);
            row.push_back(forward);
        }
    }
    return row;
}

}  // namespace

bool projections_refute(const ModularField& field, const std::vector<ModularMatrix>& slices,
                        std::size_t products) {
    const std::size_t axis = slices.size();
    if (axis < 3) return false;  // the projection has nothing to remove

    // Yang's own first line, and it is the flattening bound on this axis wearing
    // another name: fewer terms than slices cannot span them.
    if (products < axis) return true;

    const std::size_t inner_target = products - axis + 2;
    const std::vector<std::vector<int64_t>> points = projective_points(field, axis);

    const std::size_t width = axis * (axis - 1) / 2;
    linear_algebra::SpanBasis<ModularField> admitted(field, width);

    for (std::size_t first = 0; first < points.size(); ++first) {
        for (std::size_t second = first + 1; second < points.size(); ++second) {
            const std::vector<ModularMatrix> pencil{combined(field, slices, points[first]),
                                                    combined(field, slices, points[second])};
            // The inner question, and the whole reason this is affordable: two
            // slices is a pencil, so it is a canonical form and not a search.
            if (pencil_rank_of(field, pencil).over_closure > inner_target) continue;

            admitted.try_add(wedge(field, points[first], points[second]));
            if (admitted.dimension() == width) return false;  // spans everything already
        }
    }

    // The admitted planes wedge to less than the whole exterior square, and they
    // are a superset of those that could qualify, so nothing qualifies.
    return admitted.dimension() < width;
}

}  // namespace pencil_rank
