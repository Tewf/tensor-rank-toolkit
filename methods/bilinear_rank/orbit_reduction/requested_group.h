#pragma once

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "automorphism.h"
#include "group_construction.h"
#include "symmetry_argument.h"
#include "bilinear_rank_aliases.h"

/// The group a `--symmetry` choice names, for the map in hand.
///
/// It sits beside the group it constructs but outside the library target,
/// because this is the point where a word typed at a shell becomes a group and
/// the library has no business knowing that a command line exists. The three
/// commands that take the flag now live in three different folders, so the only
/// place all of them reach is here, next to `group_construction.h`.
namespace bilinear_rank {

/// How many invertible `order × order` matrices there are over GF(p):
/// `∏(p^order − p^i)` for `i < order`.
///
/// Counted in floating point on purpose. The number is only ever compared
/// against a ceiling to decide whether building the group is sane, and at the
/// sizes where the answer is no it has already overflowed anything exact.
inline double general_linear_order(double characteristic, std::size_t order) {
    double power = 1.0;
    for (std::size_t index = 0; index < order; ++index) power *= characteristic;

    double total = 1.0;
    double subtracted = 1.0;
    for (std::size_t index = 0; index < order; ++index) {
        total *= power - subtracted;
        subtracted *= characteristic;
    }
    return total;
}

/// Above this, `auto` refuses instead of building. `⟨2,2,2⟩` over GF(2) needs
/// 36 pairs and `⟨3,3,3⟩` needs 28 224; a 4×4 map over GF(2) would need four
/// hundred million, which is a refusal rather than a wait.
inline constexpr double kLargestAmbientGroup = 1e6;

/// The **ambient** group a choice names, not the stabiliser of any map.
///
/// The distinction matters and the two searches want opposite things.
/// `minimise_rank_up_to_symmetry` takes an ambient group and re-derives the stabiliser
/// every time its map moves, because a quotient taken once goes stale the moment
/// the map changes. `expand_subspace_up_to_symmetry` takes a group that already stabilises
/// the span it is given, and a group that does not is the one way it can report
/// a false `NO`. So this returns the ambient group and each caller narrows it as
/// its own search requires; handing the stabiliser to the first would silently
/// weaken it, and handing the ambient group to the second would break it.
///
/// Empty when nothing was asked for, which every search here reads as "try every
/// candidate".
inline std::vector<Automorphism> requested_ambient_group(const Field& field,
                                                         const std::vector<Matrix>& slices,
                                                         const cli::Symmetry& symmetry) {
    if (symmetry.kind == cli::SymmetryKind::None || slices.empty()) return {};

    if (symmetry.kind == cli::SymmetryKind::MatrixMultiplication) {
        // Before the group is built, not after: a group for the wrong shape is
        // undefined against this tensor rather than merely useless.
        require_matmul_shape(slices, symmetry.shape[0], symmetry.shape[1], symmetry.shape[2]);
        return matrix_multiplication_symmetry_generators(field, symmetry.shape[0],
                                                         symmetry.shape[1], symmetry.shape[2]);
    }

    const std::size_t rows = slices.front().rows();
    const std::size_t columns = slices.front().columns();
    const double characteristic = static_cast<double>(field.characteristic());
    const double ambient =
        general_linear_order(characteristic, rows) * general_linear_order(characteristic, columns);

    if (ambient > kLargestAmbientGroup) {
        std::ostringstream complaint;
        complaint << "--symmetry auto would have to build about " << ambient
                  << " automorphisms for a " << rows << "x" << columns << " map over GF("
                  << field.characteristic()
                  << "), which is a refusal rather than a wait; use --symmetry matmul <n> <m> <k> "
                     "if this is a matrix multiplication tensor, whose orbits are known in closed "
                     "form and need no group built at all";
        throw std::runtime_error(complaint.str());
    }
    return all_automorphisms(field, rows, columns);
}

}  // namespace bilinear_rank
