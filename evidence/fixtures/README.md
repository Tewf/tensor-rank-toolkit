# Fixtures

Four polynomial multiplication maps and the operators the sparsification strand
runs on, written out in full so the code is checked against bytes rather than
against a generator that could drift with it.

Each is the tensor of polynomial multiplication: `A(x)` with `n` coefficients
times `B(x)` with `m` coefficients over `GF(p)`, giving `n+m-1` output
coefficients. Slice `i` is the matrix of the bilinear form producing `c_i`, so it
carries a 1 at `(j,k)` exactly when `j+k == i`. Computing every product
separately costs `n*m` multiplications, which is the rank to beat.

The format is deliberately dull: `field p`, `shape slices rows cols`, then the
slices as dense rows separated by blank lines, `#` for comments. The shift
structure is meant to be visible when you open the file.

## What the descent reaches, and what each step costs

Produced by `evidence/reproduce/measure.py`, one core, fastest of three. Times are
cumulative, so each step includes the ones before it.

| Fixture | Field | Product | Naive | Step 1 | Step 2 | Step 3 |
|---|---|---|---|---|---|---|
| `f2_5x5` | F2 | 5×5 | 25 | 16 · 0.0001 s | 14 · 0.0009 s | **14** · 0.0985 s |
| `f2_3x8` | F2 | 3×8 | 24 | 19 · 0.0001 s | 16 · 0.0041 s | **15** · 0.207 s |
| `f2_4x7` | F2 | 4×7 | 28 | 19 · 0.0001 s | 16 · 0.0019 s | **16** · 0.397 s |
| `f3_3x6` | F3 | 3×6 | 18 | 12 · 0.0017 s | 11 · 0.0228 s | **10** · 2.69 s |

The three steps are the greedy smallest basis (1), rank minimisation over the
rank-one maps already inside `T` (2), and rank minimisation over the full
generated set `G` of rank-one maps (3).

## What that table actually says

**Step 3 improved the answer in two of the four cases, and cost one to two
orders of magnitude more than the first two steps together to do it.** The
computed range is 36 to 189 times, and it is deliberately not quoted that way:
steps 1 and 2 now finish in 0.9 ms to 23.8 ms, and a hand run moved one fixture's
ratio from 189 to 85 with nothing changed in the code, which is the rule
[`../../MEASURING.md`](../../MEASURING.md) states for a denominator that small. On
`f2_5x5` it
spent 0.0985 seconds to confirm the 14 that step 2 already had; on `f2_4x7`,
0.397 seconds to confirm 16. Where it did pay, it paid by one product: 16 to
15 on `f2_3x8`, 11 to 10 on `f3_3x6`.

So step 3 accounts for essentially all of the cost and about half of the answer,
at a price per product that no other step here comes close to. Any continuation
that only makes step 3 faster is optimising the part that mostly does not pay.
That is the finding these fixtures exist to hold still.

## Using them

The expected counts live here as well as in the test code, so a test that
disagrees with this table is a visible disagreement rather than a silent one.
Every step-3 target is asserted, including `f3_3x6`'s 10; the four are tagged
`slow` so a developer can skip them with `ctest -LE slow`, and CI runs them
anyway.

Six further fixtures ship for a published number rather than for a measurement
made here, so their shapes, naive costs and targets live under the same rule one
file along, in [`published-targets.md`](published-targets.md): two more field
extensions, cyclic convolution of length 7, and the three matrix multiplication
formats `[wang2026]` leaves open.

## The ones that are not ours

[`plinopt/`](plinopt/README.md) is thirteen `.sms` operators copied unmodified from
PLinOpt's `data/`, under **CeCILL-B**, with a copy of that licence beside them.
They are here so the exchange with the reference toolchain is exercised against
his bytes rather than against something written here to resemble them, which is
the same argument the rest of this directory rests on one step further out.

## The one that is not for a number

`f5_3x3.tensor` ships for a *test*, and is the only fixture here that does.
Everything else here is over GF(2) or GF(3), so the general-field leaf, which
walks a subspace by its base-`p` digits in
[`subspace_walk.h`](../../methods/bilinear_rank/exhaustive/subspace_walk.h), was only ever
exercised at one odd prime, and a walk right at `p = 3` and wrong at `p = 5` had
nothing to fail against. Written by `make-tensor --polynomial 5 3 3`, like the
rest. Its rank is 5, which is `n+m-1` and so optimal by Winograd's bound,
reachable by interpolation because GF(5) has at least `n+m-2 = 4` elements;
`decide-rank` closes it in one node.

## The one that is not concise

Every other `.tensor` here has all three flattenings at full rank, so it is
concise and there is nothing for
[`tensor_compression.h`](../../core/linear_algebra/tensor_compression.h) to compress
away. `nonconcise_matmul_2x2x2.tensor` is the exception, and it exists so that
the compression is tested on a tensor it actually changes rather than only on the
identity map. It is `matmul_2x2x2` with one dependent row, two dependent columns
and three dependent slices appended: `7 x 5 x 6` on the outside, flattening ranks
`(4, 4, 4)`, and compressing it gives `matmul_2x2x2` back byte for byte, so the
core's rank is the 7 that fixture is known to have. The file's own comment block
says which combination each appended position is.
