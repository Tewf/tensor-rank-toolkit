#pragma once

#include <vector>

#include "types.h"

/// The greedy sparsifier: build the change of basis one row at a time, each row
/// the sparsest combination independent of the rows already chosen.
///
/// `[beniamini2020, Alg. 6]`, which the article reports as beating both exact
/// oracles on several operators while guaranteeing nothing.
/// It differs from [the oracles](oracle_sparsifier.h) in what it minimises:
/// they count zeros, this counts `nnz + nns`, so an entry that is 4/9 costs
/// more than an entry that is 1.
///
/// **Where this departs from the article, and why.** Its line 4 is an `argmin`
/// over every vector of the space, which the authors solve by encoding the
/// objective as a MaxSAT instance and calling Z3. There is no SMT solver here
/// and adding one for this would be a large dependency for one line. What is
/// implemented instead is exact for a smaller claim: among the vectors the
/// article's own validator can produce, take those with the most zeros, then
/// take the best scalar multiple of them. That second step is a genuine
/// optimum, because scaling is the only freedom left once the zero set is
/// fixed, and it is what the oracles never do.
///
/// So this is a heuristic bounded by a heuristic, and it is labelled as such:
/// it does not reproduce the article's numbers, it implements the article's
/// objective.
namespace matrix_sparsification {

/// The scalar multiple of `vector` with the fewest entries outside `{0, ±1}`.
///
/// Only the reciprocals of the entries can turn one into a sign, so the
/// candidates are `±1/v_i` over the nonzero entries, and checking all of them
/// is exact rather than a search. Ties keep the earliest, which makes the
/// result independent of how the entries were ordered.
std::vector<Element> best_scaling(const Field& field, const std::vector<Element>& vector);

/// Algorithm 6, on the transposed operator the oracles also work on.
///
/// Takes and returns `rows`, so it is interchangeable with
/// `sparsify_bottom_up` and `sparsify_top_down` at the call site.
Matrix greedy_sparsify(const Field& field, Matrix rows);

}  // namespace matrix_sparsification
