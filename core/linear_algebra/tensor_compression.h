#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "matrix_ops.h"
#include "rank_decomposition.h"
#include "row_space_coordinates.h"
#include "tensor_flattening.h"

/// Conciseness compression: the smaller tensor a rank-deficient tensor already
/// is, and the way back from a decomposition of it.
///
/// [`tensor_flattening.h`](tensor_flattening.h) detects the situation. When the
/// axis-`d` flattening has rank `r_d < n_d`, that axis carries `n_d` vectors
/// spanning only `r_d` dimensions, and the tensor is an `r_0 x r_1 x r_2` tensor
/// written into a larger space. **Compressing to that space does not change the
/// rank**, and the two directions are why:
///
/// - a decomposition of the core expands term for term into one of the original,
///   which is `expand_decomposition` below, so `rank(original) <= rank(core)`;
/// - a decomposition of the original, read only at the kept positions of each
///   axis, is a decomposition of the core with the same number of terms, so
///   `rank(core) <= rank(original)`.
///
/// The first direction is the one that has to exist for compression to be worth
/// anything: without it a search decides the rank of a tensor nobody asked about.
///
/// Compressing first is what makes an exponential search cheaper, and by how much
/// is arithmetic on the axis lengths rather than a hope: the rank-one pool a tree
/// search walks holds `(p^n_0 - 1)(p^n_1 - 1)/(p - 1)` maps, and a `k`-term SAT
/// encoding carries `k(n_0 + n_1 + n_2)` operand variables over `n_0 n_1 n_2`
/// equations. Replacing each `n_d` by `r_d` is a change of problem size, not a
/// constant factor. What it does not change is the target `k`, so the subspace
/// walk at a leaf still costs `p^k`. `[yang2025]`'s search compresses at *every
/// node* of its recursion, which is a large part of why it is fast, and this is
/// the whole-tensor operation only; keys are
/// [`../../references.md`](../../references.md).
///
/// **What was read, and what was not.** The idea to compress is that paper's and
/// is cited to it. What was read here: this repository's entry for `[yang2025]`
/// in [`../../references.md`](../../references.md), which is detailed about Algorithm 1
/// and the pruners, and the paragraph in
/// [`tensor_flattening.h`](tensor_flattening.h) that named this gap. **Not read:
/// the paper itself, at either venue**, so no numbered result of it is claimed
/// for anything below and none should be added without opening it. The
/// construction here is in any case not a result of that paper: rank is unchanged
/// by an invertible change of basis on each axis, which is the definition being
/// applied, so it is derived here and cited to nobody.
namespace linear_algebra {

/// A tensor's concise core, and the matrices that carry a decomposition of the
/// core back to the tensor it came from.
template <class Field>
struct ConciseCompression {
    /// The core: `r_2` slices of `r_0` rows and `r_1` columns, where `r_d` is the
    /// axis-`d` flattening rank of the tensor compressed.
    std::vector<MatrixOver<Field>> slices;

    /// Axis `d`: an `n_d x r_d` matrix of full column rank, carrying a vector in
    /// the core's axis-`d` coordinates to the original's. Three of them, and the
    /// identity on an axis that was already full rank, so a concise tensor comes
    /// back unchanged and with the identity on every axis.
    std::vector<MatrixOver<Field>> expansion_by_axis;
};

/// The concise core of `slices`, and the three expansions back.
///
/// The core is the tensor restricted to a maximal independent set of positions on
/// each axis, taken greedily in index order, and the expansion of axis `d` is
/// every position of that axis written over the kept ones.
///
/// **Why the three axes can be chosen on the original tensor and restricted all
/// at once.** Restricting one axis to positions that already span it leaves the
/// relations along the other two exactly as they were: a dropped position is a
/// combination of kept ones, so a relation that holds at the kept positions holds
/// at the dropped ones too, and no relation is created or destroyed. So a set
/// independent before the restriction is independent after it, which is also why
/// the core's flattening ranks are its three sizes: it is concise.
template <class Field>
ConciseCompression<Field> compress_to_concise(const Field& field,
                                              const std::vector<MatrixOver<Field>>& slices) {
    ConciseCompression<Field> compression;
    compression.expansion_by_axis.resize(3);
    if (slices.empty()) return compression;

    std::vector<std::vector<std::size_t>> kept_by_axis(3);
    for (std::size_t axis = 0; axis < 3; ++axis) {
        compression.expansion_by_axis[axis] = coordinates_over_independent_rows(
            field, flattening(field, slices, axis), kept_by_axis[axis]);
    }

    compression.slices.reserve(kept_by_axis[2].size());
    for (const std::size_t slice : kept_by_axis[2]) {
        compression.slices.push_back(select_columns<Field>(
            select_rows<Field>(slices[slice], kept_by_axis[0]), kept_by_axis[1]));
    }
    return compression;
}

/// A decomposition of the core, as a decomposition of the tensor it came from.
///
/// Term for term: term `j` of the core is `a_j (x) b_j (x) c_j` in the core's
/// coordinates, and the term it stands for in the original's is
/// `E_0 a_j (x) E_1 b_j (x) E_2 c_j`. Summing those over `j` is the original
/// tensor, because applying one matrix per axis is linear in the tensor and the
/// three axes commute, so the sum passes through it unchanged. **The number of
/// terms does not change**, which is the whole point: an upper bound on the rank
/// of the core becomes one on the rank of the original.
///
/// A factor holds one row per term, so a term's vector is a row and the expansion
/// is applied on the right, transposed. Getting that the wrong way round is how
/// to be subtly wrong here and still return something of a plausible shape, which
/// is why the widths are checked against the core's rather than assumed.
template <class Field>
RankDecomposition<Field> expand_decomposition(const Field& field,
                                              const ConciseCompression<Field>& compression,
                                              const RankDecomposition<Field>& core) {
    if (core.factor_by_axis.size() != 3 || compression.expansion_by_axis.size() != 3) {
        throw std::invalid_argument("a decomposition and a compression each have three axes");
    }
    RankDecomposition<Field> expanded;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const MatrixOver<Field>& factor = core.factor_by_axis[axis];
        const MatrixOver<Field>& expansion = compression.expansion_by_axis[axis];
        if (factor.columns() != expansion.columns()) {
            throw std::invalid_argument(
                "expand_decomposition: the decomposition is not in the core's coordinates. Axis " +
                std::to_string(axis) + " has " + std::to_string(factor.columns()) +
                " positions against a core of " + std::to_string(expansion.columns()));
        }
        expanded.factor_by_axis.push_back(multiply(field, factor, transpose<Field>(expansion)));
    }
    return expanded;
}

}  // namespace linear_algebra
