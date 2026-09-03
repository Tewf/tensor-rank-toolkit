# Can the search generate its candidates instead of enumerating them?

The tree branches over the rank-one pool: 225 maps at 4x4, 261 121 at 9x9,
**4 294 836 225 at 16x16**. Asked whether the candidates that matter can be read
off `V` itself, so the pool never has to be walked.

Write `ones(V)` for the rank-one elements of `V`, `R(V)` for their span, and
`deficit(V) = dim V - dim R(V)`. The leaf test is `deficit(V) == 0`.

## The lemma is true, and it is an equality rather than an inclusion

**If a rank-one `m` outside `V` strictly reduces the deficit, then some `v` in
`V` has rank 2 and splits as `v = w + m` with `w` rank one.** Over GF(2), in
four steps:

1. `R(V) ⊆ V` and `m ∉ V`, so `dim(R(V) + <m>) = dim R(V) + 1`.
2. `V' \ V = m + V`, so the new rank-one elements are the `q = v + m` of rank
   one, and `v = 0` gives `q = m`. Hence `R(V') ⊇ R(V) + <m>` and
   **the deficit never rises**.
3. It falls strictly exactly when some such `q` lies outside `R(V) + <m>`.
4. For that `q`, `v = q + m` is a sum of two rank-one maps so `rank(v) <= 2`.
   `rank(v) = 0` gives `q = m`, excluded by 3; `rank(v) = 1` puts `v` in
   `ones(V) ⊆ R(V)` so `q ∈ R(V) + <m>`, excluded by 3. So `rank(v) = 2`.

Because step 3 is an *iff*, carrying its side condition along turns the
inclusion into an equality: the deficit-reducing maps are **exactly** the
summands `m` of a rank-2 `v = w + m` whose partner `w` avoids `R(V) + <m>`.

Brute-forced over GF(2) with **zero violations**: every subspace of the ambient
space for `f2_2x3` and `gf4_multiplication`, and `matmul_2x2x2` exhaustively to
depth 3 (872 086 nodes, 1.93e8 pairs) with depth 4 from a uniform 2.3% sample of
depth-3 parents. Over `GF(p)` the same argument gives `v = w + λm`, which is the
same statement once the pool is normalised one map per scalar class; **only
GF(2) was tested**.

## It does not remove the pool, and the reason is structural

Step 4 forces `w ∉ R(V)`, so **both summands of a witnessing rank-2 element lie
outside `V`**. The cheap supply of rank-2 elements, pairwise sums of the
rank-one basis already in hand, is exactly the useless kind: measured at depth 3
of `matmul_2x2x2`, 4.5 of the 27.9 rank-2 elements per node split inside `V` and
23.0 are the useful sort.

So generating the candidates means finding low-rank elements of a matrix space,
which is MinRank, `[buss1999]`, NP-complete and the same question the leaf asks.
Walking `V` costs `p^dim V`, which beats `|pool|` only while `dim V` is below
about 32 at `<4,4,4>` and is the crossover
[`rank_one_basis.h`](rank_one_basis.h) already makes per call.

**The pool enumeration stays.** The lemma licenses a generator; it does not
supply a cheap one.

## What did fall out of it, free and unrelated

`m' ∈ m + V` gives `V + <m'> = V + <m>`: the same child, reached once per pool
element of that coset. Since a child is handed `[chosen, |pool|)`, the branch
from the smaller index covers the larger one's subtree entirely, so skipping the
later twin loses nothing. Two candidates share a child exactly when they reduce
to the same residue against `V`, which is the work `contains` already does.

Measured at the root: `f2_2x3` collapses 18 live candidates to **3** children,
`matmul_2x2x2` 225 to 198, `matmul_2x2x3` 945 to 882. It grows with depth as `V`
does, reaching 1.60x by depth 3 on `matmul_2x2x2`.

**It is worth nothing at `<4,4,4>` and that is arithmetic, not luck.** The
collapse is `|pool|` over the number of cosets the pool meets, and with a
256-dimensional ambient space against a pool of `2^32` the maps are spread one
per coset. It pays where the ambient space is small beside `V`, which is the
polynomial fixtures, and not where the pool is large.
