# What it is worth, and where to point it

On the polynomial fixtures, `|Stab(T)|` is about 6 over `F₂`, a single-digit
constant at the top of the tree and less below. That turns the seven-hour
`--target 12` into one or two hours. Useful; not a change of kind. And per
[`known_ranks.md`](../../descent_search/known_ranks.md), that run now reproduces a published
exclusion rather than settling anything.

**Point it at matrix multiplication instead.** ⟨2,2,2⟩ has the sandwich
symmetries and the cyclic one: order in the hundreds over `F₂`, against a
225-element pool. That is where orbits collapse a search rather than trim it,
and [`famous_tensors/`](../../famous_tensors/README.md) is where the open
questions are.

## The heuristic is a separate question

Reducing step 3's pool to orbit representatives is one line once the machinery
exists, but it is **not** answer-preserving: `minimise_rank` is
first-improvement with irreversible pruning, so a different pool is a different
walk. It cannot produce a *false* claim, since every result is rebuilt and checked
with `spans_all`, so it is the safe place to experiment. It is not the place
the proof lives. Do the exact search first.
