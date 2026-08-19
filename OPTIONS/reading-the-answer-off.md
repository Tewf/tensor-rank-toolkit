# Reading the answer off

The two tools that do not search a candidate pool for their answer, or do not
search at all. Precedence and `BILINEAR_TUNABLES`:
[`../OPTIONS.md`](../OPTIONS.md).

## `decide-rank-by-pencil`

Takes a tensor path and `--help`. The Kronecker canonical form of a two-slice
tensor, exactly and in polynomial time, and the rank bound it gives.

There is nothing else to choose, which is the point: there is no budget, no
schedule and no pool, so nothing is tunable. On a 4x4 pencil it answers in about
50 microseconds.

**Its exit code is the unusual part and is deliberate.** `0` when the rank is
settled, which is when the pencil is diagonalisable over GF(p); **`3`
(Undecided) when the answer is only a bound.** Ja'Ja's formula is a theorem over
an algebraically closed field and falls short over a small one, so reporting the
closure value as a rank would be wrong. A sweep can therefore tell what this
settled from what it only bounded without parsing a sentence. See
[`../pencil_rank/README.md`](../pencil_rank/README.md) for the twelve pencils
that fix the boundary, three of which the classical formula gets wrong.

## `factor-over-canonical-basis`

| Flag | Default | What chose the default |
|---|---|---|
| `--floor k` | `rank_lower_bound` of the tensor | Measured: it is the maximum of the flattening bound and both rank sums, and it raises GF(16) from 4 to **8** for the price of milliseconds. |
| `--node-limit n` | `search_node_limit`, `5000000` | As `decide-rank`. Reaching it makes the answer an upper bound rather than the rank, and `components` says so. |
| `--route auto\|exhaustive\|sat\|canonical` | `auto`, which takes the solver past **20 000** pool matrices | Measured. On `matmul_2x2x2` the solver is 0.54 s and 6.0 MB against the pool's 1.01 s and 11.9 MB; on `<4,4,4>` the pool is refused outright at 8.2 TiB against a 2.0 GiB budget while the solver starts. On `gf16_multiplication` the pool route returns 12 as an upper bound in 9 m 52 s where the solver returns the rank, 9, in 2 m 12 s. [`../canonical_factorisation/routes.md`](../canonical_factorisation/routes.md). `canonical` is McKay augmentation and never the default: **16.2x fewer nodes** for 15x the wall clock, because the parent test walks all 216 group elements per node. The fix is refinement-based labelling, which neither module has. [`../canonical_factorisation/canonical-augmentation.md`](../canonical_factorisation/canonical-augmentation.md). |
| `-s, --symmetry none\|auto\|matmul <n> <m> <k>` | `auto`, which infers a product shape when the dimensions allow | Measured, and it corrected a defect. `auto` alone enumerates the ambient group and **refuses** on any 4x4 map over GF(2), about four hundred million automorphisms, so `matmul_2x2x2` ran unquotiented at 1.077 s against 1.037 s for `none`: the same search twice. The inferred closed form quotients by 6 automorphisms and takes it to **0.195 s, 5.3x**. Inferring is safe rather than merely lucky: `stabiliser_of` keeps only the elements that actually fix `span(T)`, so a wrong guess is a small group and never a false refusal. [`../canonical_factorisation/narrowing-the-search.md`](../canonical_factorisation/narrowing-the-search.md). |

**Where symmetry costs rather than pays**, also measured: on the microsecond
fixtures building the group is most of the run, `f2_2x2` going from 47 to 117
microseconds. The default is still `auto` because the instances anyone waits for
are the other kind.

**Two narrowings that do not pay** and are therefore not options: quotienting the
pool by the full `GL(n) x GL(m)`, which cannot work because that action is
transitive on rank-one matrices and the pool is one orbit; and branching on
cosets of `span(T)`, which measured **1.00x** on `matmul_3x3x3` and
`gf16_multiplication`, the two shapes it would be wanted for.
