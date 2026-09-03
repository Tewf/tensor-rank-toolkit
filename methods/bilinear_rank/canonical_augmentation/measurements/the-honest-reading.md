# The honest reading

The commitment applies to two of these five fixtures. A cube is GF(2) only and its
representatives are the closed-form orbits of `⟨n, m, k⟩`, so on `gf16`, `f2_5x5` and
`f3_3x6` the finder is a descending sweep of plain oracle calls with one candidate.
The general orbit route does not rescue them: the ambient group for a 5x5 map over
GF(2) is refused at about `10^14` elements, and a polynomial multiplication tensor's
has about four, which quotients nothing.

No known answer came out wrong. `⟨2,2,2⟩` gives 7 and `gf16` gives 9, both exact, and
`f2_5x5` gives 15, which is a true bound and simply a weak one: `[bdez2012]` settled
that rank at 13, and 12 to 14 is this repository's own bracket rather than what is
known. [`../../descent_search/known_ranks.md`](../../greedy_heuristic/known_ranks.md).

What is worth keeping is not the finder: it is the descending schedule, which hands
`find_rank` a bracket needing one refutation instead of one per rank, the zero-term
drop, which is a correctness fix, and the shared base encoding in `decide_rank`.

The canonical route quotienting `⟨2,2,2⟩`'s own pool, for scale against the group
sizes above:

```sh
enumerate-subspaces evidence/fixtures/matmul_2x2x2.tensor --target 6 -s matmul 2 2 2 --canonical
  pool: 225 rank-one maps, group: 6 elements
  canonical: 0 distinct subspaces from 0 paths, 58 nodes, 0 group visits, 0.0105228 s
```
