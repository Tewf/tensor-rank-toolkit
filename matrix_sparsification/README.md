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
sparsify-operator operator.sms --emit sparser.sms   # and hand the minimum on
```

**What the problem is and what is proved hard about it**, including the four
names one question goes under and why the exact method here does not contradict
any of the hardness results:
[`what-is-hard-about-it.md`](what-is-hard-about-it.md).

As in the other strand, the filenames carry what each method guarantees, and
[`method/README.md`](method/README.md) says what each is and where it fails.
**A third route does not search at all.** `--simplex` answers by linear
programming, which is the minimum where the operator's matroid is regular and an
upper bound elsewhere. It is the only route that finishes `4x4x4_49_156_L`, and
on the `Grey-221` operators it reaches the proved minimum about fourteen times
faster than the search does:
[`method/when-the-matroid-is-regular.md`](method/when-the-matroid-is-regular.md).

**Two methods search**: [`method/exact-over-q.md`](method/exact-over-q.md),
which returns the minimum over every invertible `V` rather than a good answer,
and the greedy by rescaling, which minimises `nnz + nns` instead and is the only
one that does. Three others reached the same counts 88x to 343x more slowly and
moved to a branch: [`dominated.md`](dominated.md). Keys are
[`../references.md`](../references.md).

The operators do not have to be typed in. [The rank
search](../descent_search/README.md) emits them, which is the whole pipeline: 25
multiplications become 14, and the additions the multiplication count never saw
come down too.

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out
sparsify-operator out_L.sms           # 31 nonzeros become 27
```

## Results

The source article reports no measured result for this operator: it says only
that the program took too long for a simple problem. It has one now, with the
full numbers in [`results.json`](results.json). Below, nonzeros and beside them
`nnz + nns`, the cost the articles minimise, which counts an entry that is not
`0` or `±1` twice because it needs a multiplication as well as an addition.

| Operator | As given | Row basis | Oracle | Greedy |
|---|---|---|---|---|
| Strassen `U` (7×4) | 12 · 12 | **10** | **10** · 10 | **10** · 10 |
| Strassen `V` (7×4) | 12 · 12 | **10** | **10** · 10 | **10** · 10 |
| Alternative basis (7×4) | 21 · 42 | **10** | **10** · 20 | **10** · **10** |

All four reach 10 nonzeros in milliseconds. Ten is what the construction
predicts rather than a surprise: inverting a square block of rows makes four rows
singletons and the remaining three carry two each. It is not proved minimal.

**The nonzero count was hiding the result.** On the alternative-basis operator
every method reaches ten nonzeros, and they are not the same ten: the oracles
leave all ten as ninths, twenty operations, and the greedy leaves ten signs, ten
operations. Half the cost, invisible to the column the strand used to report.
## What that is worth

Fewer additions lowers the leading coefficient of the arithmetic complexity, not
the exponent: Strassen's 18 additions become 12, taking the 7 of
`7·N^log₂7 − 6·N²` to **5**. Why that is the ceiling for `⟨2,2,2⟩` and where the
live record sits for `⟨3,3,3⟩`: [`what-it-is-worth.md`](what-it-is-worth.md).

## The methods

Four of them, one page each, with what each is for and where it fails:
[`method/README.md`](method/README.md). The greedy by rescaling is the only one
that wins on `nnz + nns`, and until recently no tool ran it.

## What was corrected

Three defects and what each cost, in
[`what-was-corrected.md`](what-was-corrected.md). A corrected number with no
record of the correction reads exactly like one that was always right.

## Where this stops

The exact method walks column subsets with an upper bound and no lower bound, so
it stops on collecting a basis rather than on a proof that nothing lighter
exists. **Brouwer-Zimmermann** `[zimmermann1996]`, coding theory's standard
algorithm for the same subproblem, prunes with a lower bound from several
disjoint information sets, and is the first thing to try on an operator this
cannot finish.

The decomposition of `[beniamini2019]`, which factors an operator into a sparser
one times a basis change and recurses, is still not implemented. What is
implemented is the cost model that says what such a decomposition would be
worth, which is the half that was missing when the complexity of a sparser
operator could not be stated at all.

`[plinopt]`, the reference implementation, reaches sparsity by a different route
entirely: sparse QLUP elimination and bounded coefficient search rather than the
Ω-valid oracles. Nothing here is compared against it yet, and that comparison is
the obvious next measurement.
