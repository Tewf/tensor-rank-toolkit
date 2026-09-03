#pragma once

#include <cstddef>
#include <vector>

#include "bilinear_rank_aliases.h"
#include "exhaustive_search.h"

/// The general field's half of a leaf: test every element of the subspace for
/// rank one, rather than test every rank-one map for membership.
///
/// Which of the two a leaf takes is decided in
/// [`rank_one_basis.h`](rank_one_basis.h) and is not decided here; over GF(2)
/// the same walk is done in words by [`gf2_leaf.h`](gf2_leaf.h) and this file is
/// not reached at all. What is here is the walk over `GF(p)` for every other
/// `p`, in the two forms named below.
///
/// **They exist as two so that the fast one can be checked against the plain
/// one**, on the same span, in
/// [`tests/test_gray_walk_leaf.cpp`](tests/test_gray_walk_leaf.cpp). That is the
/// same arrangement `test_packed_generation.cpp` makes for the two GF(2)
/// generation routes and for the same reason: a walk that formed an element
/// wrongly would not crash, it would decide a rank wrongly and every number
/// downstream would inherit it.
namespace bilinear_rank {

/// Up to `needed` independent rank-one maps among the `elements` elements of
/// `span`, visited in reflected Gray code order.
///
/// **This is the route that ships.** Consecutive elements differ by one basis
/// row added or subtracted, so forming one costs `O(width)` field additions and
/// no multiplication, against `O(dim * width)` multiply-accumulates to rebuild
/// it from the digits. See [`reflected_gray_walk.h`](../descent_search/reflected_gray_walk.h) for
/// the order and for why its successor is loop-free.
///
/// **The order of the elements changed when this replaced the rebuild, so the
/// particular rank-one basis handed back can differ from the one that used to
/// be.** That is a choice among equally valid answers and not a change of
/// answer: the leaf's verdict is whether `needed` independent rank-one maps
/// exist in the span, which no reordering can move, and a caller is owed some
/// rank-one basis rather than a nominated one. Nothing in the suite pins a
/// decomposition from here: every assertion downstream is on a *count* of
/// products, on each product being rank one, and on the products together
/// computing the map, all three invariant under the order.
///
/// The all-zero element is the string the walk starts on and is never tested,
/// exactly as the rebuild never tested index zero. `budget->may_examine` is
/// asked once per element examined, counting `1 .. elements - 1` as before, so
/// a leaf is abandoned after the same number of elements whichever order they
/// came in.
///
/// **What the order is worth.** One core, `taskset -c 2`, fastest of three, the
/// protocol of [`../MEASURING.md`](../MEASURING.md), with the two routes
/// alternated inside one process on one span so that they meet the same machine:
///
/// | span | elements | rebuild | Gray walk |
/// |---|---|---|---|
/// | GF(3), 3x6, dim 12 | 531 441 | 624.5 ns | **247.6 ns** |
/// | GF(5), 3x3, dim 9 | 1 953 125 | 379.4 ns | **217.7 ns** |
/// | GF(7), 3x3, dim 7 | 823 543 | 349.6 ns | **220.9 ns** |
///
/// 2.52x, 1.74x and 1.58x an element. Six runs of each spread under 2%, well
/// inside the 13% the chassis varies by, and the gaps are far outside it.
///
/// **The `dim` term is what went, and that is visible rather than argued.** Over
/// GF(3) at 3x6 the rebuild costs about 32 ns more per element for every
/// dimension added (576 ns at dim 10, rising through 606, 627 and 668 to 704 ns
/// at dim 14), while the Gray walk sits between 248 and 262 ns across all five
/// and shows no slope at all. That is the `O(dim * width)` against the
/// `O(width)`, read off the clock.
///
/// **The table was taken before the per-element test changed, and both of its
/// columns carry what that test then cost.** Most of an element was the `Matrix`
/// formed to ask its rank and the Gaussian elimination run on it (an
/// allocation for the matrix, a `SpanBasis`, a copy per row into it and a walk
/// to the last row of a question settled at the second), which both routes paid
/// in full and neither reduced. Neither is formed now. The element is tested
/// where it already lies, in the combination buffer, by
/// [`is_rank_one`](../linear_algebra/measures.h), which allocates nothing and
/// stops at the first entry that disagrees, and a `Matrix` is built only for an
/// element that is kept, which almost none are.
///
/// **Same verdicts, same counts**: `is_rank_one` is `rank == 1` and is held
/// against it over every small matrix in
/// [`tests/test_rank_one_predicate.cpp`](tests/test_rank_one_predicate.cpp).
/// What the two columns would read now has not been measured, so the figures
/// above stand as the comparison they were taken for (between the two ways of
/// forming an element, where the difference is the `dim` term and nothing else),
/// and not as what a general-field leaf costs today.
std::vector<Matrix> by_walking_the_subspace(const Field& field, const ReducedBasis& span,
                                            std::size_t rows, std::size_t columns,
                                            std::size_t needed, std::size_t elements,
                                            SearchBudget* budget);

/// The same answer, rebuilding each element from its base-`p` digits.
///
/// The route that used to ship, kept because it is the obvious reading of "every
/// element of the subspace" and so is worth having as the thing the fast one is
/// held against. Not called by the search.
std::vector<Matrix> by_rebuilding_each_element(const Field& field, const ReducedBasis& span,
                                               std::size_t rows, std::size_t columns,
                                               std::size_t needed, std::size_t elements,
                                               SearchBudget* budget);

}  // namespace bilinear_rank
