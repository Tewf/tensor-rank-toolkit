# Moving a decomposition instead of building one

Both searches next door build a decomposition from nothing. This one starts from
a decomposition that already works and walks it, which is a different guarantee:
it can only ever produce upper bounds, and it never runs out of moves.

| File | Role |
|---|---|
| [`flip_graph.h`](flip_graph.h) | The flip graph of [`[kauers2023]`](../../../references.md): a flip rewrites two terms into two others without changing what they compute, so every vertex is a valid scheme |
| [`plateau_search.h`](plateau_search.h) | New here. A walk allowed to cross plateaus, filed apart because it guarantees something different from the heuristic's steps 2 and 3 |

## The command

    walk-scheme evidence/fixtures/f3_3x6.tensor --from 10 --flips 20000

`--from k` walks from the heuristic's scheme at `k` products rather than from the
naive one, and refuses when the heuristic cannot reach `k`. That refusal matters:
walking from a bad start is what makes a flip walk look worse than it is.

## The published numbers

[`results.json`](results.json) holds them, and `evidence/reproduce/measure.py --check`
re-derives every count in it on each CI run. Five rows: the `⟨2,2,2⟩` plateau
crossing at a 380-state budget and its negative control at 370, and the flip
walk on `⟨2,2,2⟩`, GF(16) and `⟨3,3,3⟩`.

They exist because these numbers were prose before. `⟨2,2,2⟩` crossing to 7 was
quoted in three documents at 0.11 s, a figure that is `walk-scheme`'s and was
printed beside a claim about `plateau_search`; and no run anywhere regenerated
either. What `plateau_search` actually needs is a 380-state budget, at which it
visits 386 subspaces, against a default of 200 000 that reaches the same 7 over
66 063 subspaces and takes 4.56 s.

## What the start point is worth, measured

On `f3_3x6`, four seeds of 20 000 flips **from the naive scheme reach 12
products; from the heuristic's 10 they hold 10**. So the walk does not recover
what the descent already found, and the two are complements rather than rivals:
the descent reaches a good scheme, the walk explores sideways from there.

## Where this stops

A walk gives upper bounds only, and a plateau crossing is not a proof of
anything. Nothing here decides a rank; that is
[`../exhaustive_search/`](../exhaustive/README.md). And the published flip-graph
results are far ahead of this implementation, which
[`../positioning/`](../../../writeup/positioning/README.md) states with the numbers.
