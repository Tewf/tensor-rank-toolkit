#pragma once

#include <string>

#include "integer_programme.h"

/// The model as fixed-column MPS, which is the one dialect every solver here
/// reads.
///
/// The format is IBM's, from the Mathematical Programming System/360, and its
/// eighty columns are a punched card. `[mps360]` is the manual that defines it
/// and `[murtagh1981]` is the description most bibliographies point at; keys
/// are [`../references.md`](../references.md), which says which parts of each
/// were read and which were not.
///
/// Free-form MPS was the first choice and does not survive contact: CBC parses
/// the `BOUNDS` section by character position whatever the rest of the file
/// looks like. Fixed columns cost eight-character names, which is why what gets
/// written is `x1` and `c1` rather than whatever the caller called things. The
/// six fields begin at columns 2, 5, 15, 25, 40 and 50.
///
/// Three traps, all found by running the solvers rather than by reading about
/// them, and all of them silent:
///
/// - **an integer variable with no stated upper bound is binary** to CBC and to
///   GLPK, and unbounded to lp_solve. The same file then means two different
///   problems. Every variable therefore has both its bounds written out, `PL`
///   and `MI` included, and no default is ever leaned on.
/// - the integrality markers belong in fields 3 and 5, not 4 and 6. Their own
///   name goes in field 2, `'MARKER'` in field 3 and `'INTORG'` or
///   `'INTEND'` in field 5, and every quoted keyword is eight characters so
///   that it fills its field exactly. CPLEX's own manual says field 4, counting
///   whitespace-separated tokens in a format it no longer parses by column;
///   that is why this was settled by running the solvers.
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
