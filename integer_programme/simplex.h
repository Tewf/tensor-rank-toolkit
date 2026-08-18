#pragma once

#include <vector>

#include "integer_programme.h"
#include "standard_form.h"

/// The exact optimum of a linear programme, with integrality ignored.
///
/// Two-phase simplex under Bland's rule, in rationals. The method is
/// `[dantzig1951, Ch. XXI]`, whose own summary is that it constructs *"first a
/// feasible, and then a maximum feasible, solution"*; the two-phase form with
/// an explicit phase I, which is what runs here, is `[dantzig1955, p. 193]`.
/// Keys are [`../references.md`](../references.md).
///
/// The pivoting rule is `[bland1977, Thm. 1.1]`: under the smallest-subscript
/// choice on both the entering and the leaving variable, the simplex method
/// cannot cycle and is therefore finite. It takes more pivots than the textbook
/// steepest-edge choice, which is the right trade here twice over: the
/// instances are small, and a solver that loops for ever is worse than one that
/// plods. The arithmetic being exact, there is no tolerance to tune and no
/// degenerate pivot that is only degenerate to fifteen digits.
namespace optimisation {

struct LinearOptimum {
    Status status = Status::Exhausted;
    std::vector<Number> values;  // in the standard form's own columns
};

LinearOptimum solve_relaxation(const StandardForm& form);

}  // namespace optimisation
