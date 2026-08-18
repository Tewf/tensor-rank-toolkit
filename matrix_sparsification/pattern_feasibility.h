#pragma once

#include <cstddef>
#include <vector>

#include "multivariate_polynomial.h"
#include "types.h"

/// Whether a wanted pattern of zeros is reachable at all, decided rather than
/// searched for.
///
/// Every other method in this folder *finds* a sparse operator. This one
/// answers the prior question: given that you want these entries zero, is there
/// an invertible change of basis that does it? The two oracles cannot ask it,
/// because they only ever walk from the operator they were handed towards
/// whatever they can reach.
///
/// The construction is `[dumas2024cex]`, a Maple worksheet whose name is short
/// for "counterexample, polynomial determinant": pin as many rows of
/// the operator as it has columns, write the wanted pattern as unknowns, and the
/// change of basis is then forced, `P = (Q·L)^{-1}·(Q·A)`. Its determinant is a
/// polynomial in those unknowns, and the pattern is reachable exactly when that
/// polynomial is not identically zero. It is called a counterexample because
/// the pattern it settles is one the row-basis heuristic does not find.
///
/// **This decides reachability, not optimality.** A feasible pattern is one
/// some basis realises; nothing here says it is the sparsest pattern that is.
namespace matrix_sparsification {

/// What the determinant said, and what follows from it.
struct PatternVerdict {
    /// The determinant of the forced change of basis, expanded exactly. It is
    /// the obstruction: zero means no invertible basis realises the pattern.
    Polynomial obstruction;

    bool realisable = false;

    /// Entries of `L·P` that are zero whatever the unknowns are given. These
    /// are the zeros the pattern actually buys, which is not the same as the
    /// zeros it asked for: pinning rows forces zeros elsewhere too, and that is
    /// the whole reason the worksheet is interesting.
    std::size_t guaranteed_zeros = 0;

    /// An assignment of the unknowns making the determinant nonzero, and what
    /// the operator becomes under it. Empty when `realisable` is false, and
    /// also when the grid below was exhausted without a hit, which is reported
    /// rather than hidden.
    bool witnessed = false;
    Matrix change_of_basis;
    Matrix result;
};

/// Decide the pattern.
///
/// `pinned_rows` names as many rows of `operator_` as it has columns, in the
/// order they are to be pinned; `Q·L` is those rows and must be invertible.
/// `free_entries` is that many rows by that many columns, true where the target
/// is left unknown and false where it is required to be zero.
///
/// Throws when the shapes disagree, when the pinned block is singular, or when
/// the order exceeds what an exact expansion can be asked for. A refusal naming
/// the number is a result; the cofactor expansion is factorial and running it
/// on a large operator is not a thing to discover from the outside.
PatternVerdict decide_pattern(const Field& field, const Matrix& operator_,
                              const std::vector<std::size_t>& pinned_rows,
                              const std::vector<std::vector<bool>>& free_entries);

/// The largest square block this will expand a determinant over. Eight is
/// 40 320 cofactor terms, which is seconds; nine is ten times that.
std::size_t maximum_pinned_order();

}  // namespace matrix_sparsification
