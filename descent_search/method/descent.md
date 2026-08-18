# The three steps of the descent

## Step 1 is exact, and it is the only step that is

Choosing a basis of `span(T)` minimising `Σ rank` is a matroid problem:
independence of vectors forms a matroid, and the greedy (sort by weight
ascending, keep whatever stays independent) yields a **minimum-weight basis**
(Rado-Edmonds, `[oxley, Lem. 1.8.3]`; for this problem by name,
`[nakatsukasa2017, Thm. 2.1]`, whose Cor. 1 is stronger). So step 1 does not
approximate anything, and its result is
tie-break independent. Everything after it relaxes the constraint that the
answer be a basis of `span(T)`, and that is where the guarantee goes.

## Step 1: greedy smallest basis

```
minimum_weight_basis(T):
    C ← [ Σᵢ cᵢ·Tᵢ  for c ∈ GF(p)^k, c ≠ 0 ]        # p^k − 1 candidates
    sort C by (rank, enumeration index)
    B ← ∅
    for M in C:
        if B is full (|B| = dim span T): break
        if M ∉ span(B): B ← B ∪ {M}
    return B
```

Ties break on enumeration order, with the first coordinate varying fastest,
pinning this choice to ensure reproducible results.

| | |
|---|---|
| Time | Θ(p^k · (k + d)·w), building and ranking every element of the span, then Θ(p^k·k·log p) to sort |
| Space | **Θ(p^k · w)**: it materialises the whole span |

## Steps 2 and 3: minimise over a candidate pool

Identical code; they differ only in the pool `G` they are handed.

```
minimise_rank(T, G):
    loop:
        span ← basis(T)
        for i in 0 … |G|−1:
            if G[i] ∈ span: continue
            V ← minimum_weight_basis(T ∪ {G[i]})
            if cost(V) < cost(T):
                T ← V ; span ← basis(T) ; continue      # keep going down this G
            else:
                G ← [ g ∈ G[i+1…] : g ∉ span(V) ]        # drop what V already reaches
                restart loop
        return T                                          # a full pass changed nothing
```

`improving_candidates` has the same shape but never updates `T`; it just
collects the candidates that would individually pay, as a pre-filter.

Every restart replaces `G` with a strict suffix of itself, so there are at most
`|G|` restarts and at most `|G|` candidates examined per pass.

| | |
|---|---|
| Time | O(\|G\|² · p^(k+1) · (k+d)·w) worst case |
| Space | Θ(p^(k+1)·w + \|G\|·w) |

The worst case is very loose, because the pruning step is doing the real work.
The shortlists `improving_candidates` actually returned on the four fixtures
were **0, 1, 0 and 6** out of pools of 961 to 4732, so almost every candidate
is discarded before `minimum_weight_basis` is ever called on it.

**Step 2's pool** is the rank-one maps already inside `T`: `rank_one_candidates`
decomposes each slice, giving `Σ rank(Tᵢ)` candidates, which is `cost(T)`.

**Step 3's pool** is every rank-one map of the shape, one per scalar class:

> `|G| = (p^n − 1)(p^m − 1) / (p − 1)²`

built as outer products of vectors normalised to leading entry 1, in
Θ(\|G\|·w) time and space. That gives 961, 1785, 1905 and 4732 for the four
fixtures, which is what the tool reports.
