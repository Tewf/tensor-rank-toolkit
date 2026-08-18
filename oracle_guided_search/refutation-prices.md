# What a pinned refutation costs, and which mitigation paid

Measured 2026-08-17, one build, the build lock held for the whole run. Fixture
`⟨2,2,2⟩` over GF(2), five orbit candidates, question `rank <= 6`, which is the
refutation the strict test needs. Solver kissat throughout.

## The baseline on record does not reproduce

The figures the proposal was assessed against are three refusals at **30.7, 28.4 and
44.1 s**, with the whole lower-bound proof at **25.8 s**, and the objection built on
them is that each refusal costs more than the proof it sits inside.

Those refusals were taken from a CNF emitted **without** the term ordering the
whole-instance run had, with cube literals appended by hand afterwards. Matched, on
the in-repository cube path:

| question | measured |
|---|---|
| whole instance, `k = 6`, `--break-symmetry` | **1.32 s** |
| five pinned cubes, `k = 6`, no ordering | **12.14 s**, 1.63 to 2.67 s each |
| five pinned cubes, `k = 6`, `--break-symmetry` | **1.65 s**, 0.29 to 0.35 s each |

So the whole-instance proof is 20x cheaper than recorded and a pinned refusal is
about 100x cheaper. **Nothing below should be read as an improvement on the recorded
numbers**; the chain starts at the first matched measurement.

The reference moves the other way and is reported as measured rather than reconciled:
`find_rank` determining `rank(⟨2,2,2⟩) = 7` outright, with `--break-symmetry` and the
naive ceiling, asks four questions in **2.22 s** against 0.62 s on record. Both
directions of disagreement have the same cause: a timing without its flags beside it
is not a measurement.

## The mitigations, each measured against the one above it

| # | mitigation | per candidate | all five | gain |
|---|---|---|---|---|
| 0 | matched flags, no ordering | 1.63 to 2.67 s | 12.14 s | the honest start |
| 0b | `--break-symmetry`, ordering terms 1 onward | 0.24 to 0.35 s | **1.26 to 1.65 s** | 7.4x |
| 4 | `--refuter tree`, the quotiented tree | 0.0043 to 0.0099 s | **0.025 to 0.048 s** | 34 to 51x |
| 5 | `--parallel`, twelve threads | unchanged | **0.018 s** wall | 2.6x |

Ranges on rows 0b and 4 because both were measured twice, the second time as a
reproducibility check; the spread is about 2x, so nothing here is good to better than
one significant figure. Cumulative 12.14 s to 0.018 s, roughly **700x**. The tree also
beats the *whole-instance* refutation, 0.025 s against 1.32 s, by about 50x, which is
the comparison that matters: pinning is no longer a tax on the work.

On the accepting side at `k = 7`, where the strict step ends:

| refuter | candidate 0 | candidate 1 | total |
|---|---|---|---|
| solver | refuted, 1.22 s | accepted, 0.44 s | 1.66 s |
| tree | refuted, 0.80 s, 4584 nodes | accepted, 0.39 s, 2150 nodes | **1.18 s** |

Both return a verified 7-product algorithm through `recovers_map`.

## Why the tree wins, and where it stops

Not luck. Pinning **raises the dimension the tree starts from**, so it deletes a
level. `span(T) + t` has dimension 5 at `⟨2,2,2⟩` against a target of 6, so one level
remains and the 45 to 72 nodes reported are the entire search. Pinning does nothing
comparable to a CNF instance's difficulty, which is why the solver route gains
nothing from it.

The advantage therefore scales with `k - dim(span(T) + t)`, and it is gone as soon as
that is large. Measured: at `⟨3,3,3⟩` with `k = 23` the depth is 13, and the tree
returned no verdict on any of the thirteen candidates in **723 s** before it was
stopped, against a 5 000 000 node budget per candidate. This is a result about shallow
pinning near the rank, not about matrix multiplication in general.

## Mitigations not built, with reasons

- **Incremental solving through IPASIR**, expected the largest gain. Not built.
  `libcadical-dev 1.7.4-1` and `libcryptominisat5-dev` are both in the Ubuntu archive
  and neither is installed. A `pkg_check_modules` line would break the other two
  branches building in this repository until they install it too, so it belongs behind
  an optional `find_package(... QUIET)` seam decided in one place. It also needs hit
  rate measured, not only speed: an incremental solver is often weaker per call than a
  fresh one, so it can lose while looking faster.
- **Yang's rank-table pruners.** Not duplicated on purpose: `yang-search` is building
  `ranksum` and the shared `ranks[v]` table now. `FinderSettings::floor` is the seam.
- **Memoisation.** No question is asked twice at this depth, so it has nothing to
  return. It earns its place only once a sweep re-asks a tightened question.
