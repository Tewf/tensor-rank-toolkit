# Where the descent stops, and what stops it

## The finding: no single map strictly improves matrix multiplication

Three matrix multiplication tensors run through the whole descent, three times no
improvement whatsoever, while the same descent takes cyclic convolution from
25 to 10 and GF(16) from 16 to 9. `⟨3,3,3⟩` matches through steps 1 and 2; its
step 3 was given forty-five minutes and did not finish. Projected from the
measured `⟨2,3,3⟩` scan, 4.77 ms a candidate over 32 193 of them, and scaled by
the span it enumerates and the size of each rank, it wants **about 4.2 hours**.
That scan is one independent test per candidate, so twelve threads would bring
it under half an hour; nothing here is parallel yet, and that is the next thing
worth measuring.

It is not bad luck in the search order, and the tool says why:

```
# step 3 pool: 225 rank-one maps      # step 3 pool: 225 rank-one maps
# step 3 shortlist: 0       ⟨2,2,2⟩   # step 3 shortlist: 4      GF(16)
```

**Not one rank-one map of the 225 strictly improves ⟨2,2,2⟩**, so a
first-improvement greedy has nowhere to step. Nor does one of the 945 for
`⟨2,2,3⟩`, nor one of the 32 193 for `⟨2,3,3⟩`: the shortlist is empty every
time, on every size the scan can finish. Quotienting the pool by symmetry does
not rescue it either, because an empty shortlist stays empty however it is
grouped ([`methods/bilinear_rank/flip_graph/plateau_search.h:15-20`](../../methods/bilinear_rank/flip_graph/plateau_search.h)).

The reason is structural, and it is the same property that makes step 1
trustworthy: step 1 returns a minimum-weight basis of `span(T)` and is provably
optimal for that, so 8 is the true minimum **over all bases of the span**.
Strassen's seven products are not a basis of `span(T)`, which has dimension 4;
they span a 7-dimensional space containing it. A strictly descending walk only
ever moves between spanning sets that pay off one candidate at a time, and the
road to 7 does not.

## What that bounds is the descent, not the heuristic strand

Strassen's seven are reachable from the naive eight only through maps that cost
the same, and a strictly descending walk cannot enter an equal-cost state by
construction. That is what
[`methods/bilinear_rank/flip_graph/plateau_search.h:15-26`](../../methods/bilinear_rank/flip_graph/plateau_search.h) was written
for. The two rows below reach the same crossing with a different tool,
`walk-scheme`'s flip walk restarted over independent seeds rather than
`plateau_search`'s own depth-limited backtracking:

| Tensor | Strict descent | Plateau walk | Cost |
|---|---|---|---|
| `⟨2,2,2⟩` | 8, shortlist 0 of 225 | **7** | 0.11 s |
| `⟨3,3,3⟩` | 27, no improvement | **24** | 38.1 s, `--flips 20000 --seeds 8` |

Both rows are `walk-scheme`'s, measured in
[`against-the-heuristics.md`](../../methods/bilinear_rank/canonical_augmentation/measurements/against-the-heuristics.md)
line 5 and
[`three-by-three.md`](../../methods/bilinear_rank/canonical_augmentation/measurements/three-by-three.md) line
6. On `⟨2,2,2⟩` the number to beat is 7 and not 8; on `⟨3,3,3⟩`
the walk is three products under the naive 27 where the descent is level with
it. One sideways step does not do it: from the naive eight, `⟨2,2,2⟩` needs
three additions before the count moves, which is why the plateau is searched to
a depth rather than walked across.

So the two methods here are not a fast one and a slow one. They fail and succeed
on different tensors, and matrix multiplication is where the *strictly
descending* heuristic stops: the exact search decides `⟨2,2,2⟩` at 7 and the
plateau walk exhibits 7 and 24 without deciding anything. What the descent
cannot do is take a first step; what no heuristic here can do is prove a bound.
