#pragma once

#include <vector>

#include "bilinear_rank_aliases.h"

/// Turning a decomposition back into an algorithm.
///
/// The search reports how few multiplications a map needs. That number is not
/// the deliverable: the algorithm is. This recovers it, as the three operators
/// that define it, and rebuilds the map from them so the recovery can be
/// checked rather than trusted.
namespace bilinear_rank {

/// A fast algorithm for a bilinear map.
///
/// Product `j` is `(left_j · x)(right_j · y)`, and output `i` is
/// `sum_j decode[i][j]` times product `j`. So `left` has one row per
/// multiplication, and the number of rows is the cost the search minimises.
///
/// `left` and `right` are precisely what
/// [matrix_sparsification](../matrix_sparsification/) makes sparse: nonzeros in
/// them are the additions the multiplication count does not see.
struct Algorithm {
    Matrix left;    // products x n, encoding the left operand
    Matrix right;   // products x m, encoding the right operand
    Matrix decode;  // outputs x products, combining products into outputs

    std::size_t product_count() const { return left.rows(); }
};

/// The scalar `s` with `from · s == to`, checked across the whole vector.
///
/// False when there is none, which doubles as a rank-one check: the rows of a
/// rank-one matrix are all multiples of each other.
bool scalar_multiple(const Field& field, const std::vector<Element>& from,
                     const std::vector<Element>& to, Element& scalar);

/// The rank-one map each pair of rows encodes: slice `j` is the outer product
/// of row `j` of `left` with row `j` of `right`.
std::vector<Matrix> encoded_products(const Field& field, const Matrix& left, const Matrix& right);

/// Recover `left` and `right` from rank-one slices. False if any slice is not
/// rank one.
///
/// Reads the right-hand operand from the first nonzero row, so slices with a
/// zero first row produce correct results.
bool recover_operands(const Field& field, const std::vector<Matrix>& products, Matrix& left,
                      Matrix& right);

/// Recover `decode`: how each slice of `target` is built from the products.
/// False if some slice is outside their span.
bool recover_decoder(const Field& field, const std::vector<Matrix>& target,
                     const std::vector<Matrix>& products, Matrix& decode);

/// Both at once, from a decomposition and the map it is meant to compute.
bool recover_algorithm(const Field& field, const std::vector<Matrix>& target,
                       const std::vector<Matrix>& products, Algorithm& algorithm);

/// The map an algorithm actually computes. Compare with the input to check a
/// recovery.
std::vector<Matrix> map_computed_by(const Field& field, const Algorithm& algorithm);

/// Recover the algorithm and check it: does what it computes still generate
/// `target`?
///
/// This is the question both commands ask before reporting any number, because
/// a decomposition that does not compute the map is not a cheaper algorithm,
/// it is a wrong one. On true, `algorithm` holds the recovery.
bool recovers_map(const Field& field, const std::vector<Matrix>& target,
                  const std::vector<Matrix>& products, Algorithm& algorithm);

}  // namespace bilinear_rank
