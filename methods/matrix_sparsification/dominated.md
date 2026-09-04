# What left this module, and where to find it

`main` carries what you would run. Three methods that reached the *same answer*
more slowly were moved out on 2026-08-22, with their tests, their measurements
and their documentation intact. Since 2026-08-23 they are archived on the
**`rejected-experiments`** branch under `retired/dominated_sparsifiers/`, whose
`retired/README.md` indexes everything that branch holds. Nothing was deleted,
and this page exists so that looking for them is one `git switch` rather than an
archaeology of the history.

```sh
git switch rejected-experiments   # sources and method notes, indexed
git switch --detach d3c7f3f       # the full tree with them wired in and tested
```

## What moved, and the measurement that moved it

One core, fastest of three, on the three operators of `3x3x3_23_Grey-221`. Every
method below returns **the same counts** as the one that stayed, 43 / 42 / 43.

| method | slowest operator | against the minimum | why it moved |
|---|---|---|---|
| `exact oracle, top-down` | 35.6 s | **88x slower** | same answer |
| `row-basis heuristic` | 92.3 s | **227x slower** | same answer here, and it loses on 37% of 400 random operators |
| `exact oracle, bottom-up` | 139.3 s | **343x slower** | same answer |
| **`exact, matroid greedy over Q`** | **0.434 s** | not applicable | **stayed** |
| `greedy, by rescaling` | 268.0 s | not comparable | **stayed**: it minimises `nnz + nns`, and is the only method here that does |

The rescaling greedy is slow and is not dominated, because it is answering a
different question. Speed does not order methods that optimise different things.

## What did not move, and why it matters

**The Ω validator stayed**, as [`omega_validator.h`](omega_validator.h). It is
`[beniamini2020, Def. 3.2]` and the rescaling greedy still calls it; splitting it
out is what let the oracles leave, and it is what
[`method/the-validator.md`](method/the-validator.md) was always describing.

**The two oracles are `[beniamini2020]`'s own Algorithms 3 and 4**, and its
Theorem 3.22 proves the bottom-up one optimal. They are not wrong and they are
not rejected: they are superseded by a method that returns the same answer two
orders of magnitude faster.
[`../../writeup/positioning/the-sparsification-strand.md`](../../writeup/positioning/the-sparsification-strand.md)
claims this may be the only public implementation of that paper's construction,
and that claim rests on code that now lives on a branch rather than in `main`.
**Read it as "in this repository", not "in this working tree".**

## One thing the split cost, and how it was paid

`--max-memory` stopped reaching this command. The budget used to be spent by the
removed methods, which materialised every column subset of a size before looking
at any of them; the method that stayed walks them lazily and allocates almost
nothing, so there was nothing left to refuse and
`infrastructure/run_limits/tests/check_the_limits_reach_the_commands.sh` caught it.

Restoring the old behaviour would have been the wrong repair. **The scan is now
priced by what it may walk rather than by what it allocates**, which is the thing
that actually runs away: the widest level it could reach, `C(b, w)` at
`w = (b − r + 1) / 2`. On a 23×9 operator that is about ten megabytes and passes.
What it prices to on the operator it cannot finish, and what answers that
operator instead:
[`method/where-the-scan-stops.md`](method/where-the-scan-stops.md).

## If you are looking for something here

| looking for | where |
|---|---|
| the fastest correct answer | this branch, `rational_sparsifier.h` |
| `nnz + nns` rather than `nnz` | this branch, `greedy_sparsifier.h` |
| the article's Algorithms 3 and 4 | `rejected-experiments`, `retired/dominated_sparsifiers/` |
| a sparsification heuristic to compare against | `rejected-experiments`, `retired/dominated_sparsifiers/` |
| what each cost when they were measured together | `results.json` here, and with all five methods at commit `d3c7f3f` |
