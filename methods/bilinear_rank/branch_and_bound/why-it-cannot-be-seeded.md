# Why this search cannot be started from a decomposition somebody else found

The obvious thing to want: hand `lower-the-bound` a published `k`-product
algorithm and ask it for `k − 1`. `⟨3,4,5⟩` at 47 is the case that prompts it.

**It cannot be done, and the reason is the bound rather than the infrastructure.**

## What the state is

The search holds a set of matrices spanning `span(T)`. Its cost is the **sum of
their ranks**, because a basis element of rank `r` becomes `r` rank-one products.
Its one pruning rule is

```
dim V + 1 >= best   ->   cut
```

and it is sound because a subspace of dimension `d` costs at least `d`: every
element contributes at least rank one.

So the search has room to work exactly when **cost exceeds dimension**. Twelve
matrices of average rank four cost 48 over dimension 12, and the gap of 36 is the
space the search moves in.

## What a decomposition is, measured

A `k`-product decomposition is `k` matrices of rank **one**. Cost `k`, and the
dimension of what they span is `k` too, unless the terms are dependent.

Measured on this repository's own 19-product `gf64_multiplication` scheme, the one
`lower-the-bound` found on 2026-08-22:

| | |
|---|---|
| rank sum of the 19 products | **19** |
| dimension of the space they span | **19** |
| dimension of the tensor's own slice space | 6 |

So seeding with them gives `dim V + 1 = 20 >= best = 19`, and **the bound fires
before the first move is offered**. The search would report the seed back
unchanged, having done nothing, which is worse than refusing because it looks
like an answer.

The same firing, on a fixture small enough to run end to end: the descent's own
seed is already the answer, so the root itself is where the bound cuts.

    lower-the-bound fixtures/gf4_multiplication.tensor

    GF(2), start: 3 products over 3 dimensions
    best: 3 products, rank bound 3, gap 0, verified
    # 0 nodes, 0 children costed, 0 moves offered, 0 improvements, 1 branches bounded, depth 0, largest single-move drop 0, tree exhausted

One branch bounded and no node expanded: nothing above the root was ever entered.

## Why no conversion rescues it

The gap is not a representation detail. A basis of `span(T)` with rank sum `k`
groups the `k` rank-one terms into `dim` classes that share a decoding row. A
general decomposition has `k` independent decoding rows and no such grouping, so
there is nothing to convert: the information the search's state carries is
strictly less than a decomposition, in exactly the direction that matters.

This is the same relaxation [`../descent_search/minimum_weight_basis.h`](../greedy_heuristic/minimum_weight_basis.h)
already names: requiring the answer to be a basis of `span(T)` at all is the
heuristic, and it is what the exhaustive search drops.

## What can be started from a known algorithm, and what cannot

**Can**: sparsify its operators, which is
[`../matrix_sparsification/`](../../../matrix_sparsification/README.md) and needs only
the `⟨L, R, P⟩` triple. That is where a published scheme is genuinely useful input
here.

**Can**: refute `k − 1` with [`../exhaustive_search/`](../exhaustive/what-a-node-cannot-tell-you.md)
or the solver, neither of which needs a seed, both of which need the tree to be
reachable. At `⟨3,4,5⟩` the pool is `(2^12 − 1)(2^20 − 1) = 4 293 914 625` and it
is not.

**Cannot**: improve it by one product with this search. Not for want of a flag.
