# How few multiplications does a product need?

The rank of a bilinear map is the number of multiplications needed to compute
it. Strassen doing 2×2 matrix multiplication in seven instead of eight is where
fast matrix multiplication comes from, and finding such decompositions in
general is open.

There are four ways to go at it here, and the folder names say which is which:
this one descends, [`../exhaustive_search/`](../exhaustive_search/README.md) decides,
[`../flip_graph/`](../flip_graph/README.md) moves a decomposition sideways, and
[`../integer_programme/`](../../../integer_programme/README.md) hands the question to somebody
else's solver. [`../orbit_reduction/`](../orbit_reduction/README.md) quotients the first
three by symmetry. Each guarantees something different, and each folder says what.
Every claim below is one of the four earning or failing to earn its keep.

| | Guarantee | Provenance |
|---|---|---|
| [`minimum_weight_basis.*`](minimum_weight_basis.h) | Step 1, and provably optimal for the basis it chooses: a matroid greedy (see below) | `[nakatsukasa2017]` §2, Algorithm 1 and Theorem 2.1. The `GF(p)` instantiation is new here; the algorithm and its proof are not |
| [`minimise_rank.*`](minimise_rank.h) | Steps 2 and 3. None. First-improvement, irreversible pruning | New here |
| [`gf2_span_walk.*`](gf2_span_walk.h) | Step 1's representation over GF(2), a bit an entry rather than an `int64_t`: the same span in the same order, and asserted to be the same answer | New here. The representation is [`../exhaustive_search/gf2_leaf.h`](../exhaustive_search/gf2_leaf.h)'s, applied to the loop the incumbent search lives in |
| [`exhaustive_search.*`](../exhaustive_search/exhaustive_search.h) | Complete. A "no" that ran to exhaustion is a fact about the problem | An implementation of a pre-existing published algorithm |
| [`fewest_products.*`](../exhaustive_search/fewest_products.h) | Inherits the exact search's: sweep, bisection, or from the flattening bound | Which `k` to ask it about |
| [`rank_one_basis.*`](../exhaustive_search/rank_one_basis.h) | The question at every leaf of both searches: has this subspace a basis of rank-one maps? | |
| [`span_enumeration.*`](span_enumeration.h) | | Walking the `p^k` elements of a map's span |
| [`algorithm_recovery.*`](algorithm_recovery.h) |  | Turns either answer into the algorithm ⟨L, R, P⟩ it stands for |
| [`candidate_pool.*`](candidate_pool.h) · [`map_construction.*`](../map_construction/map_construction.h) |  | The rank-one maps to search over, and the maps to search on |

## The tools

```sh
minimise-rank  fixtures/f3_3x6.tensor              # heuristic: make it better
decide-rank    fixtures/f2_5x5.tensor --target 11  # exact: is there one this small?
walk-scheme    fixtures/f3_3x6.tensor --from 10    # walk on from the heuristic's answer
```

Every flag and its default: [`../OPTIONS.md`](../../../OPTIONS.md). What the descent
guarantees, proved rather than measured:
[`../article/bilinear-rank.pdf`](../../../article/bilinear-rank.pdf), with
[`correctness.md`](correctness.md) for which of those a test would catch.
## What the heuristic reaches

F2 5x5 to **14**, F2 3x8 to **15**, F2 4x7 to **16**, F3 3x6 to **10**, against
published 13, no-solution-at-14, no-solution-at-14 and 10. **Step 3 is priced
badly**: it improved two of the four by one product each, at one to two orders of
magnitude what steps 1 and 2 cost together. Per-step times and the full table:
[`what-it-reaches.md`](what-it-reaches.md).

## What the exact search decides

Small maps outright, with proof: Karatsuba's 3, the classical 3 and 6 for GF(4)
and GF(8), **rank ⟨2,2,2⟩ = 7** in half a second, and the refutation at 12 that
[`../incumbent_search/`](../incumbent_search/README.md)'s 13 completes into
**rank(F2 5x5) = 13**. Costs, and what a spent budget does and does not mean:
[`what-the-exact-search-decides.md`](what-the-exact-search-decides.md).

## Step 1 is not a heuristic

Choosing a basis of `span(T)` with the least total rank is a **matroid** problem,
so greedy-by-ascending-weight gives a minimum-weight basis (Rado-Edmonds):
`[oxley, Prop. 1.1.1]` for the matroid and `[oxley, Lem. 1.8.3]` for the greedy,
keys in [`../references.md`](../../../references.md); the table above says who states
the same thing for this problem in particular. The `16, 19, 19, 12` are the
minima over all bases of those spans and no tie-break changes them. What is
heuristic is the *constraint* that the answer be a basis of `span(T)` at all,
which is what steps 2 and 3 relax. Proofs:
[`../article/bilinear-rank.pdf`](../../../article/bilinear-rank.pdf).

## What makes a result trustworthy

A search that quietly loses a slice reports excellent numbers, so after every
step, in the tools and not only in the tests, the result must still generate the
map it came from, and the recovered ⟨L, R, P⟩ is rebuilt and compared.

## Beyond polynomial multiplication

The same two searches on the tensors the complexity literature argues about:
**[`../famous_tensors/`](../../../famous_tensors/README.md)**. No single rank-one map
strictly improves a matrix multiplication tensor, so the step 3 shortlist is 0 of
225 on `⟨2,2,2⟩` and this descent cannot take a first step. A walk that may cross
equal-cost maps reaches 7 in 0.11 s ([`../flip_graph/`](../flip_graph/README.md)).

## Where this stops

The heuristic proves nothing optimal. The exact search proves a great deal but
only where it can finish, and from scratch it is `C(|pool|, k)`. Nothing here
settles the bilinear rank problem, which is still open.
