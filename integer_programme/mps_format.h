#pragma once

#include <string>

#include "integer_programme.h"

/// The model as fixed-column MPS, which is the one dialect every solver here
/// reads.
///
/// The format is IBM's, from the Mathematical Programming System/360, and its
/// eighty columns are a punched card. `[mps360]` is the manual usually credited
/// with that card layout and `[murtagh1981]` the description most
/// bibliographies point at.
///
/// **Neither is what the integer markers below rest on, and one of them cannot
/// be.** MPS/360 was *linear and separable* programming, which has no integer
/// variables in it, so a convention for marking them is not in its manual;
/// IBM's own `[oslmps]` sends the reader on to the MPSX/370 reference for the
/// format, and MPSX/370's MIP option is where that lineage puts them. What the
/// markers here are checked against is `[oslmps]` itself. Keys are
/// [`../references.md`](../references.md), which says of each what was read.
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
///   that it fills its field exactly. **This is now checked against IBM rather
///   than against the solvers.** `[oslmps]` tabulates the marker record as
///   field 2 the marker name, field 3 `'MARKER'`, **field 4 blank**, field 5
///   the keyword, and says *"field 5 must contain the value 'INTORG'"*; its
///   field columns are 2-3, 5-12, 15-22, 25-36, 40-47 and 50-61, which is what
///   is written here. `[lpsolve_mps]` says the same in the same words.
///
///   **CPLEX's manual saying field 4 is not a second opinion, it is a second
///   way of counting, and its own example settles it.** `[cplex_mps]` prose
///   gives *"Field 4: Keyword 'INTORG' and 'INTEND'"* and adds that *"fields 5
///   and 6 are ignored"*, but its Table 2 carries no column positions at all
///   and it says the fields *"must be separated by white space"*. On a marker
///   line only three tokens are present, so CPLEX's fourth *token* sits in the
///   fifth *column field*; and its worked example writes `'INTORG'` at the same
///   offset as the field-5 row names on the data lines around it, not at the
///   field-4 values. Same characters, two numbering conventions.
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
