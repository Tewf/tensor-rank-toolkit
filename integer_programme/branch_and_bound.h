#pragma once

#include <cstddef>

#include "integer_programme.h"

/// Integrality, bought by splitting on a variable the relaxation left in
/// between.
///
/// Solving the relaxation and branching on a fractional variable is
/// `[landdoig1960, §3]`, which is a prose description of a procedure and
/// carries no numbered results to cite. Keys are
/// [`../references.md`](../references.md).
///
/// **What runs here is the dichotomy and that is not theirs.** Land and Doig
/// branch on *equalities*, `x = ⌊v⌋` and `x = ⌊v⌋ + 1`, stepping outward to
/// further integers, so their tree is not binary. The two children used here,
/// `x ≤ ⌊v⌋` and `x ≥ ⌈v⌉`, are `[dakin1965, (6)-(7)]`. His §"Comparison with
/// Land and Doig method" states the difference in those terms: theirs *"forces
/// variables to take exact integral values rather than applying bounds"*.
///
/// The depth-first walk below, with a stack recording which child of a node has
/// been taken, is `[dakin1965]`'s list and "list marker" independently: he keeps
/// it for the reason given here, that holding the tree *"could involve excessive
/// storage requirements"*.
///
/// This is the floor of [`solver_chain.h`](solver_chain.h) and the only backend
/// whose answer is exact rather than checked: it works in the same rationals as
/// the model, so it never has to be believed. That is what makes it the right
/// last resort even though it is the slowest, and it is why a claim of
/// infeasibility is only ever made here.
namespace integer_programme {

/// `node_limit` bounds the tree. Reaching it returns `Exhausted` carrying the
/// best point found, which bounds the optimum without proving it; `Optimal` from
/// here is a proof.
Solution branch_and_bound(const IntegerProgramme& programme, std::size_t node_limit);

}  // namespace integer_programme
