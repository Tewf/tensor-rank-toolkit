# Two routes to the same factorisation

`factor-over-canonical-basis` finds the rank-one rows two ways. The
factorisation is the same object either way and is checked the same way, by
[`recovers_slices`](factorisation.h); what differs is the space it costs and
what the answer is allowed to claim. See [`README.md`](README.md) for the
formulation itself.

There are three. `--route canonical` is the third and is documented separately,
in [`canonical-augmentation.md`](canonical-augmentation.md), because what it has
to say is a negative result rather than a trade-off: it visits 53x fewer nodes
for 5.1x the wall clock, and is never the default.

`--route exhaustive` materialises the pool of rank-one maps and walks a tree
over it. `--route sat` hands the same sweep to a solver, which never enumerates
a rank-one map at all: the condition is clauses over the operand vectors, so
the space is polynomial in the shape. `--route auto`, the default, takes the
solver past 20 000 pool matrices when one is on `PATH`.

Measured on this machine, one core, at the default node limit:

| tensor | shape | pool | exhaustive | SAT |
|---|---|---|---|---|
| `matmul_2x2x2` | 4x4 over GF(2) | 225 | 7, 1.01 s, 11.9 MB | **7, 0.54 s, 6.0 MB** |
| `gf16_multiplication` | 4x4 over GF(2) | 225 | **12**, an upper bound, 1 m 31 s | **9, the rank, 2 m 12 s** |
| `<4,4,4>` | 16x16 over GF(2) | 4 294 836 225 | **refused**: 8.2 TiB against 2.0 GiB | runs |

Two rows carry the argument.

On `gf16_multiplication` the routes differ in more than speed: they return
**different answers**. The solver reports 9, the rank. The pool route reports
12, and reports it as an *upper bound*, because its node budget ran out at 9,
10 and 11 and a question nobody finished asking is not a question answered no.
That is `minimal` doing its job.

**Read that row's clock carefully.** The pool route used to take four and a half
times the solver's, and the GF(2) leaf and packed generation cut it to 1 m 31 s.
It has not won anything: it finishes first by *giving up* first, still at 12
against a true 9. What improved is the rate, 6.5x of it, and the rate is what
decides the rows a search is allowed to finish.

On `<4,4,4>` the pool route cannot begin, and says so in milliseconds; the
solver starts on the same tensor without forming anything. Neither finishes it,
because that rank is open, but one of them is in the game.

What the pool route keeps is the kind of refusal it produces. Its `NO` is a tree
this repository walked to the end, in its own code. A solver's `NO` is a
solver's, checkable as DRAT and otherwise taken on trust. `route` is recorded on
every `Factorisation` so a reader knows which sort of claim `minimal` is.

`test_canonical_factorisation` runs both routes on every fixture and requires
the same count and the same check to pass. They share almost nothing, so a shape
where they disagree is a defect in whichever is wrong. When no solver is
installed the comparison is skipped **loudly**, because a route that never ran
and is reported as agreeing is the worst outcome available.

## Why GL(n) x GL(m) cannot shrink the pool, and what can

The natural hope is to quotient the rank-one maps by the sandwiching action
`M -> mu M nu` and search over representatives. It buys nothing:
[`narrowing-the-search.md`](narrowing-the-search.md) proves the action is
transitive on the nonzero rank-one matrices, leaving one orbit and no problem.

The group that helps is the subgroup that stabilises `span(T)`, covered on the
same page. It is what `expand_subspace_up_to_symmetry` quotients by here, and
it is worth a great deal: `methods/bilinear_rank/orbit_reduction/` measures the
`<3,3,3>` pool collapsing from 261 121 to **13 orbits**.

But note what that does and does not save. The quotient prunes the **search**;
the pool is still built in full before it is pruned, so the **space** is
unchanged. `RankOnePool` in `methods/bilinear_rank/candidate_pool.h` is the piece that
would fix that, storing the left and right vectors and computing `at(i)` on
demand, which is `O(p^n + p^m)` against `O(p^n * p^m)`. The exhaustive search
carries a pool index down its recursion and has not been converted. Until it is,
the answer to "generate fewer matrices" on a large shape is not to generate them
at all, which is the SAT route.

## The rest of the schedule

Floor and ceiling are [`complexity.md`](complexity.md)'s; on `gf16_multiplication`
the floor alone raises the bound from 4 to 8 for the price of milliseconds. What
the pool route does with the stabiliser, including when it falls back, is
[`narrowing-the-search.md`](narrowing-the-search.md)'s.
