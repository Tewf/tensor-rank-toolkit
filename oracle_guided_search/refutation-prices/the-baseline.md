# The baseline on record does not reproduce

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
