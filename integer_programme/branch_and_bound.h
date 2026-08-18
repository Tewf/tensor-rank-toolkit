#pragma once

#include <cstddef>

#include "integer_programme.h"

/// Integrality, bought by splitting on a variable the relaxation left in
/// between.
///
/// This is the floor of [`solver_chain.h`](solver_chain.h) and the only backend
/// whose answer is exact rather than checked: it works in the same rationals as
/// the model, so it never has to be believed. That is what makes it the right
/// last resort even though it is the slowest, and it is why a claim of
/// infeasibility is only ever made here.
namespace optimisation {

/// `node_limit` bounds the tree. Reaching it returns `Exhausted` carrying the
/// best point found, which bounds the optimum without proving it; `Optimal` from
/// here is a proof.
Solution branch_and_bound(const IntegerProgramme& programme, std::size_t node_limit);

}  // namespace optimisation
