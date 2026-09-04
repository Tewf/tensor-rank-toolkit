# An instrument built here, measured, and retired: rank as a MILP

Brent's equations were also written here as a mixed integer programme, so a third
instrument would answer the same question as the SAT strand and the tree search on
identical instances. `[deza2023]` solves those equations by constraint programming
and the 2x2 and 3x3 cases are MIPLIB 2017 benchmarks, so the formulation was
standard; the open part was whether a MILP solver could compete on this question.
**It cannot.** Fastest of three runs on a quiet machine, seconds:

| question | answer | tree search | MILP | SAT |
|---|---|---|---|---|
| `f2_2x2` k=3 | yes | 0.00 | 0.27 | 0.01 |
| `f2_2x2` k=2 | no | 0.00 | 3.34 | 0.01 |
| `f2_2x3` k=5 | yes | 0.00 | 2.77 | 0.01 |
| `f2_2x3` k=4 | no | 0.00 | **no answer in 45 s** | 0.01 |
| `gf4_multiplication` k=2 | no | 0.00 | 1.37 | 0.01 |
| `gf8_multiplication` k=6 | yes | 0.00 | 26.76 | 0.02 |

Two to three orders of magnitude behind throughout, and it fails in 45 s a
question the tree search settles in under 0.01 s. The automorphism quotient is
worth 1.9x to 9x to it and rescues nothing. It also leaked its `cbc` badly enough
to defeat three measurement runs, so the comparison table was finally taken with
the MILP row excluded. **A MILP is the wrong instrument for deciding tensor rank**,
and the encoding is retired rather than left as a fourth column nobody would run.

The solver chain underneath it stays, because it has a consumer that suits it:
[`../../methods/curve_bounds/`](../../methods/curve_bounds/) step 3 is a genuine integer
programme of 25 variables, and there the same machinery wins. The retired encoding itself is
not in this repository to read: it was published as a squashed import, so the
commits that once held those files are not in this history and no `git log`
here will produce them. The table above is what survives of it, and it is the
part worth keeping.
