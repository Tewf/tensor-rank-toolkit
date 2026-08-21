# What the incumbent search reaches, and where it stops

Node counts, not seconds. A node is one subspace expanded and a child is one
adjunction costed, and both are exact whatever else the machine is doing, which
is what [`../MEASURING.md`](../MEASURING.md) asks of a published figure. The
runs below were taken at load 2.3 and no time is quoted from them.

Every count is **verified twice**: the tool decomposes its answer, rebuilds
⟨L, R, P⟩ and multiplies it back out against the map before printing, and the
three starred rows were then re-checked outside this repository, by reading the
emitted `.sms` files and rebuilding the tensor in a few lines of Python.

## Against the descent it starts from

`start` is `descend_from_own_basis`, which is steps 1 and 2 and nothing
exponential. `step 3` is the descent's full pool scan, from
[`../fixtures/README.md`](../fixtures/README.md) and
[`../fixtures/published-targets.md`](../fixtures/published-targets.md).

| Fixture | floor | start | step 3 | **here** | nodes | children | published |
|---|---|---|---|---|---|---|---|
| `matmul_2x2x2` | 6 | 8 | 8 | **7** | 184 | 31 915 | 7 |
| `f2_5x5` | 12 | 14 | 14 | **13** * | 80 | 54 601 | 13, `[bdez2012]` |
| `f2_3x8` | 14 | 16 | 15 | **15** | 1 | 625 | no solution at 14 |
| `f2_4x7` | 14 | 16 | 16 | 16 | 7 | 7 364 | no solution at 14 |
| `f3_3x6` | 9 | 11 | 10 | **10** | 1 | 2 569 | 10, `[bdez2012]` |
| `cyclic_f2_5` | 9 | 12 | 10 | **10** | 2 | 1 530 | |
| `cyclic_f2_7` | 12 | 15 | 15 | **13** * | 22 | 17 371 | 13 |
| `gf32_multiplication` | 12 | 17 | 16 | **14** * | 139 | 90 281 | `mu_2(5) = 13` |
| `gf64_multiplication` | 14 | 23 | 20 | **20** | 3 | 2 016 | `mu_2(6) = 15` |

**The two rows that matter are `cyclic_f2_7` and `gf32_multiplication`, and they
are the two the descent could not move.** `cyclic_f2_7`'s step 3 shortlist is
**0 of 16 129**: not one rank-one map strictly improves it, so a
first-improvement greedy has nowhere to step, and the descent's answer stands at
15. This reaches **13 in 22 nodes**, which is the published rank, and
`[wang2026]` certifies 13 from below, so the answer is exactly optimal and there
is nothing under it. **What this repository proves on its own is still only
12 ≤ rank ≤ 13**: the floor is `rank_lower_bound`'s and the 13 from below is a
citation, so closing it here means a refutation at 12 that nobody has run.

`gf32_multiplication` goes 16 to 14 in 139 nodes, one product above
`mu_2(5) = 13`, and is the other row where the descent's step 3 had stopped.

**`f2_5x5` now has both sides inside this repository.** The exhaustive search
refutes 12 in 146 402 553 nodes; this exhibits 13 in 80. `[bdez2012]` reached
the same 13 by exhaustion, in 9.65·10⁹ tests and 2.28·10⁵ seconds. Nothing here
proves 13 is the rank on its own — the refutation does that, and this only stops
the citation being what the upper half rests on.

**The descent is not beaten so much as unblocked.** Every row that step 3 could
already move is reached here in **one or two nodes**, because the first
level-lowering move a node offers is usually the one step 3 finds after scanning
961 to 4732 candidates. The rows where step 3 does nothing are where the extra
levels pay.

## The three configurations, and what each is for

    lower-the-bound <tensor> --from descent --width 4 --summand-rank 3

`--width 0` enters every child, which makes the run a branch and bound and its
`tree exhausted` a statement about the whole tree above that root. That is
affordable on `matmul_2x2x2`, 184 nodes, and on nothing larger here: `f2_5x5`
at `--width 0` did not finish in 900 seconds.

`--from basis` starts at the minimum-weight basis instead of the descent's
answer, which is a **looser incumbent and therefore a taller tree**: `dim V + 1
>= best` cuts at 24 rather than at 14. On `f2_5x5` that is what reaches 13 where
`--from descent` exhausts at 14, and on `f2_4x7` it is what made the run
unaffordable. Both effects are the same one.

`--rounds` restarts from the answer. A round that exhausted its tree exhausted
it above **its own root**, so the next round starts higher and cuts sooner. It
paid on nothing measured here, which is worth recording: every improvement above
was found in the first round.

## Where the wall is, priced

**`p^dim` rank computations per child, and `dim` is the quantity the search
deliberately raises.** `minimum_weight_basis_with` ranks only the coset the
candidate opens, so a node costs `children × p^dim` ranks and the exponent climbs
one per level. Measured shape of the failures:

| | dim at the root | dim at the bound | ranks per child at the bound |
|---|---|---|---|
| `cyclic_f2_7` | 9 | 13 | 8 192 |
| `f2_4x7` from `basis` | 10 | 17 | 131 072 |
| `gf64_multiplication` | 11 | 19 | 524 288 |

`f2_4x7 --from basis` and `gf64` are where that bites: the second reaches 20 in
three nodes and then spends minutes on each of the next three. **The generated
move set is what makes the rest affordable at all** — 20 678 moves offered over
22 nodes on `cyclic_f2_7`, against the 16 129 a *single* node would offer from
the pool — but it does not touch the `p^dim` factor, and that is the honest
scaling limit of this file.

The design that would: carry the cost as an integer instead of recomputing it,
`+1` for an adjunction and `1-r` for exchanging a rank-`r` generator. It gives up
the exactness of `cost(V)`, since an integer carried along only ever knows the
generators it was handed and not the whole span, and the whole reason the descent
works is that step 1 reads all `p^dim` of them. Not built, and not measured.
