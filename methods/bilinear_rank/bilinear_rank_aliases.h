#pragma once

#include "linear_algebra.h"

/// Finding a bilinear map's rank: how few multiplications a product needs.
///
/// Three kinds of thing live here, and the filenames say which is which:
///
/// - [`minimum_weight_basis.h`](greedy_heuristic/minimum_weight_basis.h): step 1, a matroid greedy,
///   provably optimal for what it does. Choosing a minimum-rank basis of a fixed span is
///   a matroid problem, so the greedy is exact there: `[oxley, Lem. 1.8.3]` in
///   general and `[nakatsukasa2017, Thm. 2.1]` for this problem by name. Keys
///   are [`../references.md`](../../references.md).
/// - [`minimise_rank.h`](greedy_heuristic/minimise_rank.h): steps 2 and 3, a heuristic
///   guaranteeing nothing. They relax the constraint that step 1 works under.
/// - [`exhaustive_search.h`](exhaustive/exhaustive_search.h): complete and exponential,
///   an implementation of a pre-existing published algorithm.
/// - [`algorithm_recovery.h`](algorithm_recovery.h): turning either one's
///   answer back into the algorithm it stands for.
namespace bilinear_rank {

/// One field for the whole strand: the primes in play are tiny.
using Field = linear_algebra::ModularField;
using Matrix = linear_algebra::ModularMatrix;
using Element = Field::Element;

/// A span of slices, spelled once. Written out in full it is
/// `linear_algebra::SpanBasis<linear_algebra::ModularField>`, which appeared
/// five times in one function and said nothing the alias does not.
using ReducedBasis = linear_algebra::SpanBasis<Field>;

}  // namespace bilinear_rank
