#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "field.h"
#include "matrix.h"
#include "tensor_flattening.h"
#include "rank_metric_bound.h"
#include "tensor_rank_sum.h"

/// The best lower bound on tensor rank available here without searching, which is
/// the number every exponential decision procedure in this repository should
/// start from.
///
/// One function, so that adding a bound is one edit and every caller gains it.
/// There are four bounds now: the flattening ranks
/// ([`tensor_flattening.h`](tensor_flattening.h)), the line and total rank
/// sums ([`tensor_rank_sum.h`](tensor_rank_sum.h)), and Griesmer applied to the
/// slice space read as a rank-metric code
/// ([`../rank_metric_bound/rank_metric_bound.h`](../rank_metric_bound/rank_metric_bound.h)). They are kept as separate
/// functions rather than folded together because they are separately testable,
/// and because the flattening bound is polynomial time while the rank sums are not.
///
/// **On the thirteen fixtures here the flattening bound never wins**, ties once on
/// `f2_2x2`, and is kept for two reasons that are not about these fixtures: it is
/// the only one of the three that is polynomial, and it is the only one that still
/// answers when an axis is too long for a rank table. Against the *line* bound
/// alone it does win, on `f2_5x5`, 9 against 8.
///
/// Callers wire this in rather than a single method by name:
/// `methods/bilinear_rank/exhaustive/fewest_products.cpp::flattening_floor` and the SAT command's
/// floor both go through it.
namespace linear_algebra {

/// `max` over every bound we can prove cheaply. Sound by construction: each term
/// is a valid lower bound, so their maximum is one too.
template <class Field>
std::size_t rank_lower_bound(const Field& field, const std::vector<MatrixOver<Field>>& slices) {
    return std::max({flattening_lower_bound(field, slices), rank_sum_lower_bound(field, slices),
                     rank_metric_bound::griesmer_lower_bound(field, slices)});
}

}  // namespace linear_algebra
