#pragma once

#include <cstddef>
#include <vector>

#include "boolean_formula.h"
#include "tensor_file.h"
#include "types.h"

/// Håstad's reduction: 3SAT becomes tensor rank.
///
/// `[hastad1990, Lemma 2]`. A formula of `n` variables and `m` clauses becomes a
/// tensor of shape `(2 + n + 2m) × 3n × (3n + m)` whose rank is `4n + 2m` if the
/// formula is satisfiable and strictly greater if it is not. That is the whole
/// NP-hardness proof, and implementing it makes the theorem something that runs
/// rather than something that is cited.
///
/// The `3n + m` slices, in the order this builds them, with `v` the variable
/// index counting from zero and `n` the variable count:
///
/// | Slice | Nonzero at |
/// |---|---|
/// | `V_v` | `(0, 2v)` and `(1, 2v+1)` |
/// | `S_v` | `(0, 2n+v)` |
/// | `M_v` | `(0, 2v)`, `(v+2, 2v+1)`, `(v+2, 2n+v)` |
/// | `C_l` | row `0` is `u₁`; row `n+2+2l` is `u₁−u₂`; row `n+3+2l` is `u₁−u₃` |
///
/// A literal is a vector: `x_v` has a single 1 at column `2v`, and `¬x_v` has 1s
/// at `2v` and `2v+1`. The construction uses only `0` and `±1`, so it is the
/// same over every field, which is why the theorem covers all of them.
namespace satisfiability {

/// The tensor of `[hastad1990, Lemma 2]`, over `GF(p)`.
///
/// Clauses are padded to three literals first, since the construction reads
/// exactly three and padding by repetition changes nothing about satisfaction.
linear_algebra::Tensor formula_to_tensor(const Field& field, const Formula& formula);

/// `4n + 2m`, the rank the tensor has exactly when the formula is satisfiable.
std::size_t target_rank(const Formula& formula);

/// The rank-one slices Lemma 2 builds from a satisfying assignment.
///
/// `V_v⁽¹⁾` carries the satisfied literal in its first row; `V_v⁽²⁾ = V_v −
/// V_v⁽¹⁾`; `S_v` is already rank one; `M_v⁽¹⁾ = M_v − V_v⁽¹⁾`, less `S_v` when
/// the variable is false; and each clause contributes the two pieces of
/// `C_l − V_v⁽¹⁾` for a variable `v` that satisfies it.
///
/// There are at most `4n + 2m` of them, every one has rank at most one, and
/// every slice of the tensor lies in their span, which is Lemma 2's upper
/// bound, and is checkable without a solver anywhere in sight.
///
/// Throws if the assignment does not satisfy the formula, because then there is
/// no witness to build and returning a short list would look like one.
std::vector<Matrix> witness_from_assignment(const Field& field, const Formula& formula,
                                            const std::vector<bool>& assignment);

}  // namespace satisfiability
