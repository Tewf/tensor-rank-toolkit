#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "field.h"

namespace linear_algebra {

/// SMS: the sparse format LinBox and Givaro read and write, and the one the
/// original spoke.
///
/// A header line `rows columns type`, then one `row column value` triple per
/// nonzero entry with **1-based** indices, terminated by `0 0 0`. `#` comments
/// and blank lines may appear anywhere, including before the header, which is
/// where PLinOpt's shipped matrices put them.
///
/// The type letter says which ring, not which storage: LinBox writes `R` when
/// the field has cardinality zero, meaning the integers or the rationals, and
/// `M` for a finite field. Reading accepts everything LinBox accepts,
/// `M m I i R r P p`, and returns fractions either way since an integer is one.
///
/// **That is the convention, not an enforced one.** It is what both sides write
/// and what both sides accept, checked in each direction against PLinOpt's own
/// binaries. Nothing was ever handed a deliberately wrong letter, so there is no
/// evidence that a file carrying the other one would be refused. Write the right
/// letter because it tells a reader where the entries live, not because something
/// downstream will catch it.
///
/// Enables interchange with the exact-linear-algebra ecosystem: LinBox, Givaro,
/// and external solvers all speak this format. What was measured
/// travelling each way, and the four ways to get a failure that is not about SMS,
/// is [`plinopt_interoperability.md`](plinopt_interoperability.md).
RationalMatrix read_sms(std::istream& input);

RationalMatrix read_sms_file(const std::string& path);

/// Rationals and integers alike are cardinality zero, so the type is `R`.
void write_sms(std::ostream& output, const RationalMatrix& matrix);

/// Over GF(p) the type is `M`. The residue is written as it is held, in
/// `[0, p)`, which is what LinBox's own writer emits.
void write_sms(std::ostream& output, const ModularMatrix& matrix);

/// Write one operator, with `comment` as a leading `#` line.
///
/// PLinOpt's own matrices carry such a line saying which algorithm they encode,
/// and the tools on both sides skip it, so it costs nothing and a file that has
/// travelled still says where it came from.
void write_sms_file(const std::string& path, const std::string& comment,
                    const ModularMatrix& matrix);

}  // namespace linear_algebra
