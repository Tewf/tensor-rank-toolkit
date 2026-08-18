# How `A` is found, and what it costs

Write `s = dim span(T)`, `N = nm`, and `P` for the rank-one maps of the shape,
of which there are `(p^n - 1)(p^m - 1)/(p-1)^2`, so `|P|` grows like `p^(n+m)`.
The rank is `r`. See [`README.md`](README.md) for what `A` and `C` are and
[`routes.md`](routes.md) for choosing between the two searches.

## The five steps

1. **Floor.** `rank_lower_bound`: the maximum of the flattening bound and both
   rank sums. `O(k N^2)` field operations, milliseconds on everything here.
2. **Ceiling.** `sum_i rank(T_i)`, reachable by decomposing each slice alone, so
   its existence needs no search. `O(k N^2)`.
3. **Sweep.** For `t = floor, floor+1, ...` ask: *is there a `t`-dimensional
   `W` containing `span(T)` with a basis of rank-one matrices?* The first yes is
   the rank, because every question below it was refused with the tree finished.
4. **Rows.** The `t` rank-one matrices the successful question returns, flattened,
   are the rows of `A`.
5. **Recovery.** `C` by solving `T_i = c_i A` once per slice, which is Gaussian
   elimination on a `N x t` system: `O(k t N min(t, N))`, negligible beside step 3.

Only step 3 is hard, and steps 1, 2, 4 and 5 are polynomial in every argument.

## Step 3, and where the cost is

**Exhaustive route.** The tree picks `t - s` maps out of `|P|` to adjoin to
`span(T)`, and tests each leaf by scanning `P` for `t` independent rank-one maps
inside `W`. Worst case

    O( C(|P|, t - s) * |P| * t * N )   time,    O(|P| * N)   space,

so time is about `p^((n+m)(t-s+1))`: exponential in the shape and in the gap
between the floor and the rank. **The space term is the one that bites first**,
and it is what refuses `<4,4,4>` at 8.2 TiB before any search begins.

**SAT route.** The question goes to a solver as clauses over the operand
vectors, `O(t(n+m))` of them plus `O(tk)` coefficients. No pool is formed, so
space is polynomial in the shape; `<4,4,4>` is 206 800 variables and 617 728
clauses, about 11 MB. Time is the solver's, and worst case still exponential:
deciding tensor rank is NP-complete over every finite field `[hastad1990]`, so
no route here is polynomial and none claims to be.

## Against the exhaustive search

**On the pool route there is nothing to compare: it is the same search.**
`factor_over_canonical_basis` calls the same `expand_subspace` that `decide-rank`
calls. It differs by sweeping `t` from the floor instead of answering one `t`,
which costs `sum_{t=floor}^{r}` of the term above and is dominated by its last
summand, and by recovering `C`, which is polynomial. So the factorisation is the
exhaustive search **plus a polynomial afterthought**, and it neither beats it nor
claims to. What it adds is the certificate, not speed.

The comparison worth making is the SAT route against the tree.

| | exhaustive | SAT |
|---|---|---|
| space | `Theta(p^(n+m) N)` | `O(r N log p)`, **polynomial** |
| time, worst case | `p^((n+m)(r-s))` | no better bound; see below |
| a refusal is | a tree this code walked to its end | the solver's, checkable as DRAT |

**The space separation is proved and is the whole point.** It is the difference
between refusing `<4,4,4>` in milliseconds and starting on it.

**The time separation is not proved, and the direction may surprise.** The SAT
encoding carries about `r(n+m) log p` operand bits, so brute force over its
assignments is `p^(r(n+m))`. The tree searches only `p^((n+m)(r-s))`, and since
`r - s <= r` **the naive bound for SAT is no better and is usually worse**. The
reason is structural rather than incidental: the tree is handed `span(T)` and has
only to find `r - s` further maps, while the encoding solves for all `r` rank-one
terms from scratch. That is a real advantage the tree has and the encoding gives
up.

What the solver has instead is clause learning and propagation, for which the
tree has no analogue. That is why the time column is settled by measurement and
not by an exponent, and why `routes.md` reports seconds rather than a theorem.

Deciding tensor rank is NP-complete over every finite field `[hastad1990]`, so
neither route is polynomial in time and neither is claimed to be. Only the space
claim is a claim.

## Narrowing the branching

The branching factor is `|P|`, so that is what any improvement has to attack.
One approach works and is used, and two do not:
[`narrowing-the-search.md`](narrowing-the-search.md).
