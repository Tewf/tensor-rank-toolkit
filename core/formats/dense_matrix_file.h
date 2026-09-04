#pragma once

#include <istream>
#include <string>

#include "field.h"

namespace formats {

/// Read a matrix of rationals: `shape rows columns`, then dense rows whose
/// entries are integers or fractions written `4/9`. Blank lines and `#`
/// comments are ignored. Each row carries exactly `columns` entries; a row
/// with more or fewer throws.
///
/// Entries stay fractions all the way through. The operators of a fast
/// multiplication algorithm have entries like 4/9, which no double holds, and
/// the quantity being minimised is how many of them are zero.
linear_algebra::RationalMatrix read_rational_matrix(std::istream& input);

linear_algebra::RationalMatrix read_rational_matrix_file(const std::string& path);

/// Print a matrix with fractions written the way the fixtures write them.
std::string to_string(const linear_algebra::RationalMatrix& matrix);

std::string to_string(const linear_algebra::ModularMatrix& matrix);

/// Write a matrix in the format `read_rational_matrix` reads, with `comment`
/// as a leading `#` block.
///
/// This is how the rank search hands its encoding operators to the
/// sparsification: integers are rationals, so one format serves both.
void write_matrix_file(const std::string& path, const std::string& comment,
                       const linear_algebra::ModularMatrix& matrix);

}  // namespace formats
