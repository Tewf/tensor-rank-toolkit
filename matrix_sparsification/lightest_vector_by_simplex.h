#pragma once

#include <vector>

#include "types.h"

/// The lightest vector of a row space, as a linear programme rather than a walk.
///
/// **What this is for.** The scan in
/// [`method/exact-over-q.md`](method/exact-over-q.md) enumerates column supports
/// and does not finish on an operator whose greedy weights are large. Where the
/// operator's column matroid is **regular**, minimising the number of nonzeros
/// collapses to minimising the sum of absolute values on basic solutions
/// `[tillmann2019, Thm. 5]`, and the answer is a linear programme. The reasoning,
/// which operators qualify, and what it reaches:
/// [`method/when-the-matroid-is-regular.md`](method/when-the-matroid-is-regular.md).
///
/// **What it costs to be wrong about regularity.** Everything below runs and
/// returns whatever the LP says, on any operator. The result is a genuine basis
/// of the right space either way, because that is checked rather than assumed;
/// what regularity buys is that the weight is the *minimum*. On an operator that
/// is not regular this is an upper bound and nothing more, and the caller is
/// told which it holds.
///
/// **Exact, and deliberately not through the solver chain.**
/// [`../integer_programme/simplex.h`](../integer_programme/simplex.h) is a
/// two-phase simplex in exact rationals, so a vertex at `1/3` is `1/3`. The
/// external solvers reached through `solver_chain.h` print decimals, and
/// `0.33333333` fails the exact feasibility check they are then measured
/// against, so for a continuous programme the chain declines three times over a
/// subprocess each before the built-in answers anyway.
namespace matrix_sparsification {

/// What the linear programme found, and how much it is worth.
struct LightestVectors {
    /// A basis of the row space of `rows`, as its rows, lightest first.
    Matrix basis;
    /// The weight of each basis vector, in the same order.
    std::vector<std::size_t> weights;
    /// The least weight of any nonzero vector of the space. This one is the
    /// minimum whenever the matroid is regular, and an upper bound otherwise.
    std::size_t least = 0;
    /// False when the programmes could not produce a spanning set, which is not
    /// expected and means the answer must not be used.
    bool spans = false;
};

/// One programme per coordinate: `min Σ|x_i|` over the row space with `x_j = 1`.
///
/// The lightest answer over every `j` is the least weight; the answers assembled
/// greedily in ascending weight are the basis. `rows` is `r × n` and the result
/// has the same shape.
LightestVectors lightest_vectors_by_simplex(const Field& field, const Matrix& rows);

}  // namespace matrix_sparsification
