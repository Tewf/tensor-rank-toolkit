# Sparsifying the operators

A fast multiplication algorithm has two costs. The multiplications, which
[the other strand](../bilinear_rank/greedy_heuristic/) counts, and the additions, which are set
by how many nonzero entries its operators carry. Given the operator `U`, the
problem is to find an invertible `V` minimising `nnz(U V)`. The articles put it
as `nnz + nns` instead, since an entry that is not `0` or `±1` costs a
multiplication on top of its addition.

```sh
sparsify-operator evidence/fixtures/strassen_u.matrix --show
sparsify-operator operator.sms              # SMS is read directly, by extension
sparsify-operator operator.sms --emit sparser.sms   # and hand the minimum on
```

The first line above, run on this machine, printed:

```
evidence/fixtures/strassen_u.matrix
  as given: 12 nonzeros, 12 operations, 7x4
  exact, matroid greedy over Q: 10 nonzeros, 10 operations, 0.000140825 s
0 0 0 1
1 0 0 1
0 1 0 1
0 -1 0 0
0 0 1 1
1 0 0 0
0 0 1 0
```

**What the problem is and what is proved hard about it**, including the four
names one question goes under and why the exact method here does not contradict
any of the hardness results:
[`what-is-hard-about-it.md`](what-is-hard-about-it.md).

As in the other strand, the filenames carry what each method guarantees, and
[`method/`](method/) says what each is and where it fails.
**A third route does not search at all.** `--simplex` answers by linear
programming: the minimum where the operator's matroid is regular, an upper bound
elsewhere. What it reaches where no operator is regular, and why it beats the
search on every operator large enough to time:
[`method/answering-without-searching.md`](method/answering-without-searching.md)
and [`method/accelerations-not-built.md`](method/accelerations-not-built.md).

**Two methods search**: [`method/exact-over-q.md`](method/exact-over-q.md),
which returns the minimum over every invertible `V` rather than a good answer,
and the greedy by rescaling, which minimises `nnz + nns` instead and is the only
one that does. Three others reached the same counts 88x to 343x more slowly and
moved to a branch: [`dominated.md`](dominated.md). Keys are
[`../../references.md`](../../references.md).

The operators do not have to be typed in. [The rank
search](../bilinear_rank/greedy_heuristic/) emits them, which is the whole pipeline: 25
multiplications become 14, and the additions the multiplication count never saw
come down too.

```sh
minimise-rank evidence/fixtures/f2_5x5.tensor --emit-operators out
sparsify-operator out_L.sms           # 31 nonzeros become 27
```

## Results

The three routes on the shipped fixtures. Every one reaches 10 nonzeros, and the
first of them **proves** that 10 is the minimum over every invertible `V`, which
none of them could say before 2026-08-22. Beside each, `nnz + nns`, the cost the
articles minimise, which counts an entry that is neither `0` nor `±1` twice
because it needs a multiplication as well as an addition.

| Operator | As given | Minimum, over `Q` | By rescaling | By linear programming |
|---|---|---|---|---|
| Strassen `U` (7×4) | 12 · 12 | **10** · 10 | **10** · 10 | **10** |
| Strassen `V` (7×4) | 12 · 12 | **10** · 10 | **10** · 10 | **10** |
| Alternative basis (7×4) | 21 · 42 | **10** · **10** | **10** · **10** | **10** |

Full numbers, and the same three routes on operators large enough to time, in
[`results.json`](results.json).

**The nonzero count used to hide the result on the third row.** Every method
reaches ten nonzeros there and they were not the same ten: the methods that have
since moved to a branch left all ten as ninths, twenty operations, where the
greedy by rescaling left ten signs, ten. The exact method now reaches ten
operations as well, which is a measurement of one tie-break rather than a
guarantee, because it minimises zeros and breaks ties by the order it walks
supports in.

## What that is worth

Fewer additions lowers the leading coefficient of the arithmetic complexity, not
the exponent: Strassen's 18 additions become 12, taking the 7 of
`7·N^log₂7 − 6·N²` to **5**. Why that is the ceiling for `⟨2,2,2⟩` and where the
live record sits for `⟨3,3,3⟩`: [`what-it-is-worth.md`](what-it-is-worth.md).

## The methods

Three of them, one page each, with what each is for and where it fails:
[`method/`](method/). The greedy by rescaling is the only one
that wins on `nnz + nns`; the linear programme is the only one that answers an
operator too large to search. Three more moved to a branch:
[`dominated.md`](dominated.md).

## What was corrected

Three defects and what each cost, in
[`what-was-corrected.md`](what-was-corrected.md). A corrected number with no
record of the correction reads exactly like one that was always right.

## Where this stops

The exact method walks column subsets with an upper bound and no lower bound, so
it stops on collecting a basis rather than on a proof that nothing lighter
exists. Where it stops, and the published algorithm that could get past it:
[`method/where-the-scan-stops.md`](method/where-the-scan-stops.md).

The decomposition of `[beniamini2019]`, which factors an operator into a sparser
one times a basis change and recurses, is still not implemented. What is
implemented is the cost model that says what such a decomposition would be
worth, which is the half that was missing when the complexity of a sparser
operator could not be stated at all.

`[plinopt]`, the near neighbour in this problem area, reaches sparsity by a
different route entirely: sparse QLUP elimination and a bounded coefficient
search. What its runs on these operators measured, and what the exact stage is
worth handed to its subexpression pass, where minimising nonzeros turns out to be
able to cost additions, are recorded in
[`measured-with-other-tools/`](measured-with-other-tools/).
