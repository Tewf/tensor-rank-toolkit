#pragma once

#include "linear_algebra.h"

/// Tensor rank and satisfiability are the same problem.
///
/// Håstad proved it in 1990: deciding the rank of a three-dimensional tensor is
/// NP-complete over every finite field, and NP-hard over the rationals
/// (`[hastad1990]`; keys are [references.md](../../references.md)). That is a
/// statement with two directions, and this folder runs both of them.
///
/// - [`formula_to_tensor.h`](formula_to_tensor.h) is the hard direction. Any
///   3SAT formula becomes a tensor whose rank is `4n + 2m` exactly when the
///   formula is satisfiable, so tensor rank is at least as hard as SAT.
/// - The encoders are the easy direction. A decomposition is a certificate a
///   machine can check in polynomial time, so `rank(T) ≤ r` is a satisfiability
///   question, and asking a solver is how you decide rank *in* NP rather than
///   by enumeration.
///
/// The second is why this exists at all. The
/// [exhaustive search](../bilinear_rank/exhaustive/exhaustive_search.h) is complete and
/// pays for it: its own `method/` costs F₂ 5×5 at twelve products at seven
/// hours. A solver answers questions of that shape for a living.
namespace satisfiability {

/// One field for the whole folder, the same one the rank search uses: the
/// primes in play are tiny.
using Field = linear_algebra::ModularField;
using Matrix = linear_algebra::ModularMatrix;
using Element = Field::Element;

}  // namespace satisfiability
