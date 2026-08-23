# The three sparsifiers that reached the same answer more slowly

Retired from `matrix_sparsification/` on 2026-08-22, archived here on
2026-08-23 when the `dominated-methods` branch was folded into this one.
None of the three is wrong and none was rejected: every one returns the
same counts as the exact matroid greedy that stayed, 43 / 42 / 43 on the
three operators of `3x3x3_23_Grey-221`. They left because the method that
stayed proves the same answer and is faster by two orders of magnitude.

One core, fastest of three:

| method | slowest operator | against the minimum |
|---|---|---|
| exact oracle, top-down (`oracle_sparsifier`) | 35.6 s | 88x slower |
| row-basis heuristic (`heuristic_sparsifier`) | 92.3 s | 227x slower, and it loses on 37% of 400 random operators |
| exact oracle, bottom-up (`oracle_sparsifier`) | 139.3 s | 343x slower |

## Why these are still worth reading

- **The two oracles are `[beniamini2020]`'s own Algorithms 3 and 4**, and its
  Theorem 3.22 proves the bottom-up one optimal.
  `positioning/the-sparsification-strand.md` on `main` claims this may be the
  only public implementation of that paper's construction; this directory is
  that implementation.
- **The row-basis heuristic is the honest baseline**: the one method ever
  measured losing (on 37% and 21% of two random families), which is what makes
  the exact method's "the older methods were right, they just could not prove
  it" claim a measurement instead of a guess.
- `method/when-the-matroid-is-regular.md` records a regularity claim that was
  later retracted (`4x4x4_49_156_L`, invented by a sample); it stays because a
  retraction with its object deleted cannot be checked.

## The full tree, with these wired into the build and their tests

Commit `d3c7f3f` ("Answer the operator the search cannot finish, by not
searching") is an ancestor of `main` and holds the whole module exactly as it
was measured: sources, `CMakeLists.txt` registration, `test_sparsify.cpp`
coverage, and `results.json` with all five methods' timings.

```sh
git switch --detach d3c7f3f
```

The measurement that moved them, and what replaced them, live on `main`:
`matrix_sparsification/dominated.md` and `matrix_sparsification/method/exact-over-q.md`.
