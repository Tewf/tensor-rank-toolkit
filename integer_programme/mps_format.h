#pragma once

#include <string>

#include "integer_programme.h"

/// The model as fixed-column MPS, which is the one dialect every solver here
/// reads.
///
/// Free-form MPS was the first choice and does not survive contact: CBC parses
/// the `BOUNDS` section by character position whatever the rest of the file
/// looks like. Fixed columns cost eight-character names, which is why what gets
/// written is `x1` and `c1` rather than whatever the caller called things.
///
/// Three traps, all found by running the solvers rather than by reading about
/// them, and all of them silent:
///
/// - **an integer variable with no stated upper bound is binary** to CBC and to
///   GLPK, and unbounded to lp_solve. The same file then means two different
///   problems. Every variable therefore has both its bounds written out, `PL`
///   and `MI` included, and no default is ever leaned on.
/// - the integrality markers belong in fields 3 and 5, not 4 and 6.
/// - a value long enough to overrun its twelve-character field silently becomes
///   part of the next one, so only one entry is written per line and the value
///   always comes last.
///
/// Rows are scaled by the lowest common denominator of their own entries, so
/// every coefficient and right-hand side reaches the solver as an integer and
/// nothing is lost on the way in. Scaling a row changes nothing it says, and
/// scaling the objective moves the optimum by a factor that never leaves this
/// file, because the objective the caller is told is recomputed from the model.
/// The one lossy place left is a bound that is neither whole nor on a whole
/// variable, and `satisfies` is what catches the consequence.
namespace optimisation {

std::string mps_of(const IntegerProgramme& programme);

/// The name this writer gives a variable, so a caller reading a solver's output
/// can find it again.
std::string column_name(std::size_t index);

}  // namespace optimisation
