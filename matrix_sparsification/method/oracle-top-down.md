# Exact oracle, top-down

The same, walking column subsets from the largest downwards and taking the
first validator found. A vector forced to zero on more columns cannot be beaten
by one forced on fewer, so the first hit is the best and there is nothing to
gain by looking further.

```
sparsify_by_descending_support(rows):
    settled ← ∅
    repeat a times:
        for size s = b−1 down to a−1:
            for each S ⊆ [b] with |S| = s:
                if find_validator(rows, S, settled) gives a vector with a zero:
                    take it, settle that row, next round
        if nothing was taken: stop
    return rows
```

| | |
|---|---|
| Time | O( a · 2^b · (a⁴ + a·b) ) worst case, Ω( a·b·a⁴ ) when the first size hits |
| Space | Θ( C(b, ⌊b/2⌋)·b ) at the widest subset size |

Its advantage is the early exit; its exposure is that when large subsets yield
nothing it walks `Σ_s C(b,s)` of them on the way down.

## This one looks exact too, and that is worth saying carefully

**Walking down from `b−1` to `a−1` is the same walk
[`exact-over-q.md`](exact-over-q.md) makes upwards**, since a zero set of size
`s` is a support of size `b−s`. Read against `[beniamini2020]` the argument
closes: taking the first size at which any subset has a validator is taking a
**maximal Ω-valid set**, and their Lemma 3.7 says any validator of one yields an
Ω-independent vector with at least `|S|` zeros, hence an optimally sparse one. The
floor at `a−1` is correct rather than a truncation, because no greedy weight can
exceed `b − a + 1`. So this method appears to answer Problem 2.15 exactly, and
with `[gottlieb2010]`'s driver around it the assembly would be the minimum.

**That argument is read, not tested, and it is why the guarantee is claimed on
the other page and not here.** It rests on a lemma read in the paper and on
`find_validator` implementing Ω-validity, checked by reading the code rather than
by instrumenting it. What is measured is weaker and worth having anyway: over the
three `Grey-221` operators, 400 random operators and 203 with a sparse basis
hidden behind a change of basis, this method never returned more than the proved
minimum. [`exact-over-q.md`](exact-over-q.md) is the one carrying the claim
because it is the one with a test squeezing it from both sides.

Two things separate them in cost. This one materialises every subset of a size
before looking at any of it, and it calls the validator once per candidate row
rather than solving for the vanishing space once. That is where the hundredfold
goes, for the same 43 nonzeros.
