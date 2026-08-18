#pragma once

#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "pencil_divisors.h"
#include "sumi_bound.h"

namespace pencil_rank {

/// The Kronecker canonical form of a pencil, as the four things it is made of.
///
/// The form itself is never assembled. Every question the rank formula asks is
/// answered by these counts, and building the block matrix would only give
/// something else to keep consistent. `[gantmacher1959, Ch. XII §5, Thm. 5]` is
/// what says the four are enough: they determine the pencil up to strict
/// equivalence. Keys are [`../references.md`](../references.md).
struct KroneckerStructure {
    /// Degrees of a minimal basis of the pencil's null space, ascending. A zero
    /// here is a column that is zero in both slices.
    std::vector<std::size_t> column_indices;

    /// The same on the other side.
    std::vector<std::size_t> row_indices;

    std::vector<ElementaryDivisor> divisors;

    /// The size of the square regular block, which the two dimension counts
    /// determine twice over. They are required to agree.
    std::size_t regular_size = 0;
};

/// The Kronecker structure of the pencil `first + x second` over GF(p).
///
/// Throws if the two dimension counts disagree with each other or with the
/// degrees of the elementary divisors. Those are three routes to one number and
/// the arithmetic is exact, so a disagreement is a defect and never a rounding.
KroneckerStructure kronecker_structure(const ModularField& field, const ModularMatrix& first,
                                       const ModularMatrix& second);

/// What this module can say about the rank, and how strongly.
struct PencilRank {
    /// The rank over the algebraic closure of GF(p), which
    /// `[grigoriev1978, Thm. 1]` gives exactly, and `[jaja1979]` independently.
    /// Enlarging a field can only add rank-one maps to choose from, so this is
    /// a **proved lower bound** on the rank over GF(p) itself.
    std::size_t over_closure = 0;

    /// The same count taken with the elementary divisors over GF(p) itself,
    /// where an irreducible of degree at least 2 is nonlinear because it has no
    /// root here rather than because a root repeats.
    ///
    /// **PROVISIONAL.** On the twelve pencils in `README.md` it never exceeds
    /// the rank and equals it on nine, which is sharper than `over_closure` on
    /// every one of them. It is not proved to be a lower bound and it is not
    /// proved to be reached, so it is reported beside the proved number rather
    /// than instead of it.
    ///
    /// That it cannot simply be the answer is published, not merely observed
    /// here: `[sumi2009, Thm. 3.3]` carries Ja'Ja's count only under
    /// `|F| >= deg p_1(A)`, and `[sumi2009, Prop. 3.4]` is the counterexample
    /// showing that condition cannot be dropped. Its pencil is `(I_3, C)` with
    /// `C` the companion matrix of `x^3 + x + 1` over GF(2), which is in this
    /// module's own table, and the published bound there is `>= 5` against a
    /// count of 4.
    std::size_t over_the_field = 0;

    /// The best **proved** lower bound this module has: the larger of
    /// `over_closure` and `[sumi2009, Thm. 3.5]`'s `n + k` where that applies.
    ///
    /// Strictly better than `over_closure` alone. On GF(4) multiplication the
    /// closure value is 2 and this is 3, which is the rank; on `(I_4, C)` over
    /// GF(2) with `C` the companion of `x^4 + x + 1` the closure value is 4 and
    /// this is 5, against a rank of 6.
    std::size_t proved = 0;

    /// Whether `over_closure` is also the rank over GF(p).
    ///
    /// True only in the case this module can prove: no minimal indices, and
    /// every elementary divisor a linear form of multiplicity one. Then the
    /// pencil is simultaneously diagonalisable over GF(p) and the `r` matrices
    ///
    /// True in two cases now. The first is the one this module could always
    /// prove: no minimal indices, and every elementary divisor a linear form of
    /// multiplicity one, so the pencil is simultaneously diagonalisable over
    /// GF(p) and the `r` matrices `e_i e_i^T` attain the bound. The second is
    /// `[sumi2009, Thm. 3.3]`, which settles the regular case outright whenever
    /// `Card(K) >= deg p_1(A)`, and which covers pencils the first misses: GF(4)
    /// multiplication is not diagonalisable over GF(2) and is exact here.
    ///
    /// False does **not** mean the bound is loose. It means this module has not
    /// proved it tight, and over a small field it frequently is not: see the
    /// measured table in
    /// [`the-measured-gap.md`](the-measured-gap.md).
    bool exact = false;
};

/// The rank over the algebraic closure, from the structure.
///
///     rank = sum (eps_i + 1) + sum (eta_j + 1) + regular_size + delta
///
/// over the nonzero minimal indices, where `delta` counts the elementary
/// divisors of degree at least 2 **over the closure**. This is
/// `[grigoriev1978, Thm. 1]` term for term, stated there for a pair of matrices
/// over an algebraically closed field, which is the hypothesis the whole of
/// this file's caution is about. `[jaja1979]` reaches the same count
/// independently; the `eps_i + 1` per singular block is its Theorem 2.1 and the
/// regular part its Theorem 3.3, as `[sumi2009]` cites them.
///
/// An irreducible of degree `d` splits there into `d` distinct linear forms,
/// so a GF(p) divisor `p^e` contributes `deg(p)` closure divisors, each
/// nonlinear exactly when `e >= 2`.
/// That is why the count below multiplies by the base degree rather than adding
/// one per GF(p) divisor, and getting this wrong is what made an earlier
/// version of this file claim 5 for a tensor of rank 6.
std::size_t rank_over_closure(const KroneckerStructure& structure);

/// The same sum with the GF(p) elementary divisors. See
/// `PencilRank::over_the_field`: sharper on every pencil measured, and not
/// proved.
std::size_t rank_over_the_field(const KroneckerStructure& structure);

/// The rank of a tensor with at most two slices, in polynomial time.
///
/// Throws if it is handed more than two slices, which is a different problem
/// and an exponential one.
PencilRank pencil_rank_of(const ModularField& field,
                          const std::vector<ModularMatrix>& slices);

}  // namespace pencil_rank
