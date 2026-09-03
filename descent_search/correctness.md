# Which guarantees a test would catch

The guarantees themselves, with their proofs, are in
[`../article/bilinear-rank.pdf`](../article/bilinear-rank.pdf). They are stated
once, there, so that a proof is corrected in one place.

What belongs here instead is a property of this repository rather than of the
mathematics: whether a claim would still be believed after it stopped being
true. A proof in a document drifts silently from the code it describes. A test
does not.

| | Result | Checked by |
|---|---|---|
| Thm 3.1 | Step 1 returns a minimum-weight basis | not ours: `[nakatsukasa2017, Thm. 2.1]`, whose `Cor. 1` is stronger; `[oxley, Lem. 1.8.3]` is the general matroid form |
| Thm 3.2 | Soundness: the result always generates the map | `descent_guarantees`, and `verify` in the tool after every step |
| Thm 3.3 | Termination in at most `cost(S₀) − rank(T)` adoptions | argued only: the adoption count is not exposed |
| Lem 3.4 | A candidate inside the span can never improve | `descent_guarantees`, by way of Theorem 3.5 |
| Thm 3.5 | The fixed point is 1-opt against the **whole** pool | **`descent_guarantees`** |
| Thm 4.1 | Improving is invariant under the stabiliser | **`descent_guarantees`** |
| Cor 4.2 | Any subgroup of it is sound, with finer orbits | implied by Theorem 4.1's test |

## The two worth running

**Theorem 3.5** is the one most exposed to a defect, and the reason the test
exists. The loop stops when no *surviving* candidate improves; Lemma 3.4 is what
promotes that to a statement about the whole pool, and Lemma 3.4 is a claim
about `survivors_after`. If the pruning were ever wrong the descent would return
a quietly worse answer with nothing failing. So the test rescans the entire pool
at the fixed point and requires `improving_candidates` to come back empty.

**Theorem 4.1** is what the orbit strand rests on, and its test also guards the
gap Corollary 4.2 covers: the implementation quotients by the subgroup generated
by whichever supplied generators fix the span, not by the full stabiliser. The
test asserts the improving set is closed under that group, and separately that
the group is not trivial, so the check cannot pass by being vacuous.

Both run on `f2_2x2` and `f2_2x3`, the two fixtures whose ambient group
`general_linear_group` will build. Larger shapes go through the closed form and
are covered by `symmetry_agreement` instead.

## What no test can establish

There is no approximation ratio, so nothing here is checking one, **and there is
a published reason no heuristic here will ever have a good one**: approximating
3-tensor rank within `1 + 1/1852` is NP-hard over any field, `[swernofsky2018]`,
which includes `GF(p)`. That bounds every method in this repository at once, so
the absence below is structural rather than an omission. Theorem 3.5
concerns single additions: `⟨2,2,2⟩` sits at a 1-opt fixed point of cost 8 while
its rank is 7, which is a fact about the neighbourhood and not a defect.

## What a run of it looks like

`minimise-rank fixtures/f2_5x5.tensor --steps 3` prints, verbatim:

    fixtures/f2_5x5.tensor
      naive: 25 multiplications, 9 slices, 0 s cumulative
      step 1: 16 multiplications, 9 slices, 0.000107719 s cumulative
      step 2: 14 multiplications, 11 slices, 0.00126404 s cumulative
    # step 3 pool: 961 rank-one maps
    # step 3 shortlist: 0
      step 3: 14 multiplications, 11 slices, 0.106819 s cumulative
    # algorithm: 14 products, rank bound 12, gap 2, L is 14x5, R is 14x5, P is 9x14

Reaching that last line is Theorem 3.2 exercised rather than only tested:
`run` in [`commands/minimise_rank_main.cpp`](commands/minimise_rank_main.cpp)
calls `verify` after every step and `recovers_map` at the end, and any one of
them failing prints `FAILED` and stops before the algorithm line is ever
written. The `14` multiplications is the same number
[`results.json`](results.json) and
[`what-it-reaches.md`](what-it-reaches.md) publish for this fixture.
