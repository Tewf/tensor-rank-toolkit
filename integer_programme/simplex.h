#pragma once

#include <vector>

#include "integer_programme.h"
#include "standard_form.h"

/// The exact optimum of a linear programme, with integrality ignored.
///
/// Two-phase simplex under Bland's rule, in rationals. Bland's rule takes more
/// pivots than the textbook steepest-edge choice and provably cannot cycle,
/// which is the right trade here twice over: the instances are small, and a
/// solver that loops for ever is worse than one that plods. The arithmetic being
/// exact, there is no tolerance to tune and no degenerate pivot that is only
/// degenerate to fifteen digits.
namespace optimisation {

struct LinearOptimum {
    Status status = Status::Exhausted;
    std::vector<Number> values;  // in the standard form's own columns
};

LinearOptimum solve_relaxation(const StandardForm& form);

}  // namespace optimisation
