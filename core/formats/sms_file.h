#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "field.h"

namespace formats {

/// SMS: the sparse format LinBox and Givaro read and write, and the one the
/// original spoke.
///
/// A header line `rows columns type`, then one `row column value` triple per
/// nonzero entry with **1-based** indices, terminated by `0 0 0`. `#` comments
/// and blank lines may appear anywhere, including before the header, which is
/// where PLinOpt's shipped matrices put them.
///
/// One triple per line, and a value is refused if anything follows it on its
/// line. That is not house style: LinBox reads `1 1 5 3 2 7` as a single entry
/// of 5327, silently, so accepting it would mean holding a different matrix from
/// the reference tools out of the same bytes. The argument, and the run that
/// showed it, are in `sms_file.cpp`.
///
/// The type letter says which ring, not which storage: LinBox writes `R` when
/// the field has cardinality zero, meaning the integers or the rationals, and
/// `M` for a finite field. Reading accepts everything LinBox accepts,
/// `M m I i R r P p`, and returns fractions either way since an integer is one.
///
/// **The letter carries no information downstream, and that is now measured
/// rather than assumed.** PLinOpt reads every operator over the rationals
/// whatever the letter says and takes the field from `-q` on the command line
/// (`src/PMchecker.cpp:213-219`); of the 153 matrices it ships, 15 are typed `M`
/// and 9 of those hold negative entries with no modulus anywhere in the file.
/// Write the right letter because it tells a reader where the entries live, not
/// because anything downstream will catch it.
///
/// This is the interchange format with the exact linear algebra ecosystem:
/// LinBox, Givaro and PLinOpt all speak it. What was measured
/// travelling each way, and the four ways to get a failure that is not about SMS,
/// is [`interchange/`](interchange/README.md).
linear_algebra::RationalMatrix read_sms(std::istream& input);

linear_algebra::RationalMatrix read_sms_file(const std::string& path);

/// The same file over GF(p), which is where an operator has to land before
/// anything here can compute with it.
///
/// The field is a parameter because the file does not carry it: this is the
/// reading half of `write_sms(std::ostream&, const linear_algebra::ModularMatrix&)`, which had
/// none, and it is how a published algorithm becomes a tensor in
/// [`operators_to_tensor_main.cpp`](../../methods/bilinear_rank/commands/operators_to_tensor_main.cpp).
///
/// `4/9` is reduced rather than refused, since a fast algorithm's operators are
/// full of ninths and mod 2 a ninth is 1. A denominator that vanishes is refused:
/// `4/9` modulo 3 has no residue to stand for, and returning anything would be
/// answering a different question.
linear_algebra::ModularMatrix read_sms(std::istream& input, const linear_algebra::ModularField& field);

linear_algebra::ModularMatrix read_sms_file(const std::string& path, const linear_algebra::ModularField& field);

/// Rationals and integers alike are cardinality zero, so the type is `R`.
void write_sms(std::ostream& output, const linear_algebra::RationalMatrix& matrix);

/// Over GF(p) the type is `M`. The residue is written as it is held, in
/// `[0, p)`, which is what LinBox's own writer emits.
void write_sms(std::ostream& output, const linear_algebra::ModularMatrix& matrix);

/// Write one operator, with `comment` as a leading `#` line.
///
/// PLinOpt's own matrices carry such a line saying which algorithm they encode,
/// and the tools on both sides skip it, so it costs nothing and a file that has
/// travelled still says where it came from.
void write_sms_file(const std::string& path, const std::string& comment,
                    const linear_algebra::ModularMatrix& matrix);

}  // namespace formats
