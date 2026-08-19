#include "subspace_canon.h"

#include <algorithm>

#include "span_basis.h"

namespace bilinear_rank {

namespace {

/// The index of a row's first nonzero entry, which in reduced row echelon form is
/// its pivot and identifies it.
std::size_t pivot_of(const Field& field, const std::vector<Element>& row) {
    for (std::size_t column = 0; column < row.size(); ++column) {
        if (!field.isZero(row[column])) return column;
    }
    return row.size();
}

}  // namespace

SubspaceCode subspace_code(const Field& field, const std::vector<Matrix>& generators) {
    const ReducedBasis span = linear_algebra::span_of(field, generators);
    std::vector<std::vector<Element>> rows = span.rows();
    std::sort(rows.begin(), rows.end(),
              [&field](const std::vector<Element>& left, const std::vector<Element>& right) {
                  return pivot_of(field, left) < pivot_of(field, right);
              });

    SubspaceCode code;
    for (const std::vector<Element>& row : rows) code.insert(code.end(), row.begin(), row.end());
    return code;
}

CanonicalSubspace canonical_subspace(const Field& field,
                                     const std::vector<Automorphism>& group,
                                     const std::vector<Matrix>& generators) {
    CanonicalSubspace best;
    if (group.empty()) {
        best.code = subspace_code(field, generators);
        return best;
    }

    std::vector<Matrix> moved(generators.size(), Matrix());
    for (std::size_t index = 0; index < group.size(); ++index) {
        for (std::size_t which = 0; which < generators.size(); ++which) {
            moved[which] = act_on(field, group[index], generators[which]);
        }
        const SubspaceCode code = subspace_code(field, moved);
        if (index == 0 || code < best.code) {
            best.code = code;
            best.attaining.assign(1, index);
        } else if (code == best.code) {
            best.attaining.push_back(index);
        }
    }
    return best;
}

SubspaceCode image_code(const Field& field, const Automorphism& sigma, const Matrix& form) {
    const Matrix moved = act_on(field, sigma, form);
    return SubspaceCode(moved.data(), moved.data() + moved.entry_count());
}

}  // namespace bilinear_rank
