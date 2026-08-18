# Sparsifying the operators

A fast multiplication algorithm has two costs. The multiplications, which
[the other strand](../descent_search/README.md) counts, and the additions, which are set
by how many nonzero entries its operators carry. Given the operator `U`, the
problem is to find an invertible `V` minimising `nnz(U V)`. The articles put it
as `nnz + nns` instead, since an entry that is not `0` or `±1` costs a
multiplication on top of its addition.

```sh
sparsify-operator fixtures/strassen_u.matrix --show
sparsify-operator operator.sms              # SMS is read directly, by extension
```

As in the other strand, the filenames carry what each method guarantees.

| | Guarantee | Provenance |
|---|---|---|
| [`heuristic_sparsifier.*`](heuristic_sparsifier.h) | None on the minimum. It does guarantee the shape: as many singleton rows as `U` has columns | New here |
| [`oracle_sparsifier.*`](oracle_sparsifier.h) | Exact for the question each round asks, which is narrower than "sparsest operator" | The article's, `[beniamini2020, Alg. 3 and 4]` |
| [`greedy_sparsifier.*`](greedy_sparsifier.h) | None. Minimises `nnz + nns` rather than `nnz`, and wins on the operator where those differ | The article's, `[beniamini2020, Alg. 6]` |
| [`pattern_feasibility.*`](pattern_feasibility.h) | Decides reachability exactly. Says nothing about optimality | `[dumas2024cex]`, unpublished |
| [`algorithm_cost.*`](algorithm_cost.h) | | What sparsifying is worth: `[beniamini2019, Claim 2.11 and 3.9]` |
| [`algorithm_check.*`](algorithm_check.h) | | Whether a triple still multiplies matrices |
| [`combinations.*`](combinations.h) | | The `C(n, k)` both oracles enumerate |

Keys are [`../references.md`](../references.md).

The operators do not have to be typed in. [The rank
search](../descent_search/README.md) emits them:

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out
sparsify-operator out_L.sms           # 31 nonzeros become 27
```

That is the whole pipeline: 25 multiplications become 14, and the additions the
multiplication count never saw come down too.

## Results

The source article reports no measured result for this operator: it says only that
the program took too long for a simple problem. It has one now. Full numbers in
[`results.json`](results.json).

Nonzeros, and beside them `nnz + nns`, the cost the articles actually minimise.
It counts an entry that is not `0` or `±1` twice, because such an entry needs a
multiplication as well as an addition.

| Operator | As given | Row basis | Oracle | Greedy |
|---|---|---|---|---|
| Strassen `U` (7×4) | 12 · 12 | **10** | **10** · 10 | **10** · 10 |
| Strassen `V` (7×4) | 12 · 12 | **10** | **10** · 10 | **10** · 10 |
| Alternative basis (7×4) | 21 · 42 | **10** | **10** · 20 | **10** · **10** |

All four methods reach 10 nonzeros, in milliseconds. Ten is what the
construction predicts rather than a surprise: inverting a square block of rows
makes four rows singletons, and the remaining three come out with two nonzeros
each. It is not proved minimal.

**The nonzero count was hiding the result.** On the alternative-basis operator
every method reaches ten nonzeros, and they are not the same ten: the oracles
leave all ten as ninths, twenty operations, and the greedy leaves ten signs, ten
operations. Half the cost, invisible to the column the strand used to report.

## What that is worth, in the only currency that matters

Fewer additions lowers the leading coefficient of the algorithm's arithmetic
complexity, which is what decides whether a sub-cubic algorithm beats the
classical one at a size anyone runs. Strassen, every number checked rather than
quoted:

| | `q_u` | `q_v` | `q_w` | total | leading coefficient |
|---|---|---|---|---|---|
| As published | 5 | 5 | 8 | 18 | **7** |
| Sparsified here | 3 | 3 | 6 | 12 | **5** |

Seven is the 7 of `7·N^log₂7 − 6·N²`. Five is what `[karstadt2017]` reports for
the alternative-basis version, reached here from the fixtures.

**Those 12 additions are the end of the road for `⟨2,2,2⟩`, and the literature says
so.** In the standard basis 15 additions are *necessary* for any `⟨2,2,2;7⟩`
algorithm, over an arbitrary ring: `[probert1976]` and `[bshouty1995]`. That is a
bound over every rank-7 decomposition, not over one orbit, and Winograd's variant
attains it. Changing basis is what gets past it, and 12 is stated optimal there too
(`[martensson2026]`). So no search over this fixture can win, and anything that looks
like it has is a measurement error. For `⟨3,3,3⟩` at rank 23 the record is live and
currently 55 additions (`[karunaratne2026]`), against the 61 `[beniamini2020]`
reports.

Computing `q_w` needed Strassen's decoding operator, which is a fixture now, and
it is not asserted to be Strassen's: [`algorithm_check.h`](algorithm_check.h)
verifies the triple against the 2×2 product through the trilinear identity.

The third operator is a Strassen-like algorithm already written in an alternative
basis, so its entries are ninths. It is the case where floating point has
something to go wrong with, because no double holds 4/9.

## The methods

**Row-basis heuristic.** Take a square block of rows of `U` that is invertible
and use its inverse. It guarantees that as many rows of the result as `U` has
columns are singletons, and costs one pass per subset of rows.

**Exact oracle, bottom-up.** Over every column subset one smaller than the row
count, find a vector in the row space that is zero on all of them, and keep
whichever has the most zeros overall. Replace a row with it, and repeat.

**Exact oracle, top-down.** The same, but walking subsets from the largest
downwards and taking the first one found: a vector forced to zero on more
columns cannot be beaten by one forced on fewer, so there is nothing to gain by
looking further.

**Greedy.** Build the change of basis a row at a time, each row the sparsest
combination independent of those already taken, scoring `nnz + nns` rather than
`nnz`. The article solves its inner `argmin` with a MaxSAT encoding and Z3;
what is here takes the most zeros the validator can produce and then the best
scalar multiple of them, which is exact for that smaller claim and is the step
the oracles never take.

**Pattern feasibility.** Not a search. Pin as many rows as the operator has
columns, write the wanted pattern as unknowns, and the change of basis is
forced; its determinant is a polynomial, and the pattern is reachable exactly
when that polynomial is not identically zero.

All five written out precisely, with their time and space cost:
**[`method/`](method/README.md)**.

## What was corrected

**Algorithm 2.4 was unreachable.** `sparsifying_…py:268-272` offers the choice
between the two oracles and both arms call `algorithm2_3`. The top-down method
could never be run from the command line.

**Its search falls off the end**, returning `None` into `v, i = algorithm4(u)`.
It does not fire on these operators, so it was latent rather than observed.

**The search objective was computed on doubles.** This implementation counts zeros
exactly by testing field equality, not floating-point equality on raw values.
Previous approaches using `== 0` on raw floats could miscount: on the
alternative-basis operator they see 86 zeros where 144 exist, if rounding is
applied afterwards. When counts are sound but the raw objective is not, the
reported results are misleading.

Everything here is exact rationals, so none of that is possible: a zero is a
zero because it is one.

## Where this stops

Both oracles are exact for the sparsest-independent-vector subproblem, but they
assemble the answer greedily, one row at a time, and that assembly is not proved
optimal.

The decomposition of `[beniamini2019]`, which factors an operator into a sparser
one times a basis change and recurses, is still not implemented. What is
implemented is the cost model that says what such a decomposition would be
worth, which is the half that was missing when the complexity of a sparser
operator could not be stated at all.

`[plinopt]`, the reference implementation, reaches sparsity by a different route
entirely: sparse QLUP elimination and bounded coefficient search rather than the
Ω-valid oracles. Nothing here is compared against it yet, and that comparison is
the obvious next measurement.
