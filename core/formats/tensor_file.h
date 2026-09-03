#pragma once

#include <cstddef>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "field.h"

namespace formats {

/// A bilinear map, as the list of matrices of the forms producing each output
/// coordinate. Slice i of the polynomial multiplication tensor is the form
/// producing coefficient c_i.
struct Tensor {
    int64_t characteristic = 0;
    std::vector<linear_algebra::ModularMatrix> slices;

    std::size_t rows() const { return slices.empty() ? 0 : slices.front().rows(); }
    std::size_t columns() const { return slices.empty() ? 0 : slices.front().columns(); }
};

/// Read the fixture format: `field p`, `shape slices rows columns`, then the
/// slices as dense rows. Blank lines and `#` comments are ignored.
///
/// `p` must be prime, and that is checked here rather than left to whichever
/// backend the tensor reaches: see `linear_algebra::is_prime` in [`field.h`](../linear_algebra/field.h)
/// for why nothing downstream models a composite.
///
/// Throws std::runtime_error on anything it does not understand, rather than
/// returning a half-built tensor for a caller to misread.
Tensor read_tensor(std::istream& input);

Tensor read_tensor_file(const std::string& path);

/// Write what `read_tensor` reads, with `comment` as a leading `#` block, one
/// `# ` line per line of it.
///
/// This lived inside `make-tensor` for as long as one command was the only
/// thing that wrote a tensor, which left the format's two halves in different
/// layers: the reader could only ever be checked against fixtures somebody had
/// typed, never against what the repository itself emits. With both halves here
/// a round trip is a test.
///
/// The comment parameter is the one `write_sms_file` and `write_matrix_file`
/// carry, for their reason as well as one of its own. Every reader here skips
/// `#`, so a file that has travelled still says which map it holds and what
/// wrote it, and no caller has to know that a tensor comment starts with a hash.
void write_tensor(std::ostream& output, const Tensor& tensor,
                  const std::string& comment = "");

}  // namespace formats
