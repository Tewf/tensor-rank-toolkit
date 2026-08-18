# tensor-rank-toolkit

[![CI](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml)
[![Live pages](https://img.shields.io/badge/pages-tewf.github.io%2Ftensor--rank--toolkit-1f6feb)](https://tewf.github.io/tensor-rank-toolkit/)
[![Licence](https://img.shields.io/badge/licence-MIT-lightgrey)](LICENSE)

> [Lire en français](README.fr.md)

**Exact tensor and bilinear rank over finite fields.** The rank of a bilinear
map is the number of multiplications needed to compute it. Strassen's
seven-instead-of-eight for 2×2 matrices is where fast matrix multiplication comes
from, and finding such decompositions in general is open.

This attacks it from four directions: fewer multiplications, fewer additions, the
rank asked as a question a SAT solver can answer, and, where no search can reach,
a bound read off an algebraic curve. **Nothing here is ever a float**, so a
reported rank is a fact about the map rather than an artefact of rounding.

Every count below is asserted by the test suite and reproduces anywhere. Timings
are not, and are not claimed to: see [`MEASURING.md`](MEASURING.md) for the
protocol and [`reproduce/`](reproduce/) for the driver that regenerates them.

## What it computes

**[Rank by descent](descent_search/)**, the cheap direction. Three steps: an
exact matroid greedy for the starting basis, then two relaxations that trade the
guarantee for reach.

| Map | Naive | Reached | Published rank |
|---|---|---|---|
| F2 5×5 | 25 | **14** | 13, `[bdez2012]` |
| F2 3×8 | 24 | **15** | no solution at 14 |
| F2 4×7 | 28 | **16** | no solution at 14 |
| F3 3×6 | 18 | **10** | 10, `[bdez2012]` |

The guarantees are proved rather than asserted, in
[`article/bilinear-rank.pdf`](article/bilinear-rank.pdf): step 1 is exact by
Rado-Edmonds, the descent is sound and terminates, its fixed point is locally
optimal against the whole candidate pool and not merely the part it scanned, and
the orbit quotient is invariant under any subgroup of the stabiliser. Which of
those a test would catch is recorded in
[`descent_search/correctness.md`](descent_search/correctness.md).

**[Rank by exhaustion](exhaustive_search/)**, the expensive direction, which
proves things. It settles small maps outright, reproducing Karatsuba's 3, the
classical 3 and 6 for GF(4) and GF(8), and **rank ⟨2,2,2⟩ = 7** decided from the
tensor in half a second. On F2 5×5 it rules out 9, 10 and 11 products
exhaustively, so with the descent's 14 this proves **12 ≤ rank ≤ 14** here;
`[bdez2012]` report 13. On F3 3×6 both sides are proved in about 25 seconds.

**[Lower bounds without a search](linear_algebra/tensor_rank_sum.h).** Two
rank-sum bounds return a floor from the tensor alone in milliseconds, and they
are tight often enough to remove the dearest question in a sweep entirely: they
raise GF(16) from 4 to **8** and cyclic convolution from 5 to **9**, each of
which previously cost a minute of exhaustion.

**[Sparsifying the operators](matrix_sparsification/)**, which is the other half
of the cost. Strassen's encoding operators go from **12 nonzeros to 10**, and an
alternative-basis operator from **21 to 10**, in milliseconds. Fewer nonzeros
means fewer additions, the cost the multiplication count does not capture.

**[The rank question as satisfiability](satisfiability/).** Håstad proved
deciding tensor rank NP-complete over every finite field, and that cuts both
ways: `formula_to_tensor` turns 3SAT into a tensor, and three encoders turn the
rank question into one a solver answers. A refutation can be written as DRAT and
checked by `drat-trim`, so a lower bound from a solver is verifiable rather than
trusted.

**[Quotienting by symmetry](orbit_reduction/).** A change of coordinates fixing
the target subspace maps solutions to solutions, so one member of each orbit
suffices: **28× on a refutation**, and the ⟨3,3,3⟩ candidate pool collapses from
261 121 to **13 orbits**.

**[Isomorph-free enumeration](oracle_guided_search/).**
`enumerate-subspaces --canonical` is `[mckay1998]`'s canonical augmentation,
which deduplicates with no memory at all. It returns ⟨2,2,2⟩'s 36 solution
subspaces as the **1 orbit** they are, visiting **1982× fewer nodes**. Wall clock
improves only 1.6×, because finding a canonical code by walking the whole group
spends most of the saving on itself, and that is measured rather than glossed
([`deduplication-cost.md`](oracle_guided_search/deduplication-cost.md)).

**[Two slices, without a search](pencil_rank/).** A tensor with two slices is a
matrix pencil, and Kronecker's theory gives its minimal indices and elementary
divisors by exact linear algebra in **polynomial time, with no candidate pool**.
What it will not give is a rank: Ja'Ja's formula is a theorem over an
algebraically closed field, and on `(I_4, C)` over GF(2) it says 5 where the
exhaustive search **proves** 6. So the module reports a proved lower bound, a
sharper count marked provisional, and *exact* only where the pencil is
diagonalisable over the field. Twelve pencils settled by exhaustion, three of
which the classical formula gets wrong, are tabulated in
[`pencil_rank/README.md`](pencil_rank/README.md).

## The finding worth stating on its own

**The expensive step is priced badly.** Step 3 of the descent enumerates the
full pool of rank-one maps. Across the four polynomial fixtures it improved the
answer in **two of four cases**, by one product each time, and cost between
**58 and 184 times** what the first two steps cost together. Any continuation
that only makes step 3 faster is optimising the part that mostly does not pay.
The fixtures exist to hold that finding still:
[`fixtures/README.md`](fixtures/README.md).

## One pipeline

The rank search recovers the encoding operators ⟨L, R, P⟩ from its decomposition
and writes them out; the sparsification is what they are for.

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out   # 25 -> 14 multiplications
sparsify-operator out_L.sms                                 # 31 -> 27 nonzeros
```

## What is where

```
linear_algebra/          exact arithmetic over GF(p) and over Q, shared by everything
formats/                 tensor, dense matrix, SMS, DIMACS and SMT-LIB files
cli/                     what every command shares: clock, exit codes, argument
                         grammar, the stdout/stderr split, the tunables
tunables.conf            the numbers a run is bounded by, in a file not in code
testing/                 the assertion helper every module's tests use
run_limits/              how much memory and how many cores one run may take
descent_search/          rank from above, by descent
exhaustive_search/       rank decided outright, and what that costs
map_construction/        building the maps every method runs on
orbit_reduction/         quotienting all three searches by symmetry
flip_graph/              moving a decomposition sideways instead of building one
oracle_guided_search/    fixed-k search, tree refutation, canonical augmentation
canonical_factorisation/ the rank as A B, with the receipt that checks it
pencil_rank/             two slices, where the answer is read off a canonical
                         form instead of searched for
matrix_sparsification/   fewest nonzeros in an operator
satisfiability/          the same rank question put to a SAT or SMT solver
curve_bounds/            bounds from interpolation on an algebraic curve
integer_programme/       the linear and integer programme layer the curve strand uses
fixtures/                the maps and operators everything is run on
reproduce/               regenerates every published number, with its provenance
references.md            every paper cited anywhere here, by the keys the code uses
state-of-the-art.md      where the research front is, and which parts of it are here
positioning.md           what this library adds to it, and what it does not
MEASURING.md             how a timing here was taken, and what it does not mean
OPTIONS.md               every flag of every tool, its default, and what
                         measured that default; links OPTIONS/ for the tables
article/                 the write-up: definitions, theorems, proofs, negative results
```

A folder with something to say carries its own `README.md`; one without says its
purpose at the top of its `CMakeLists.txt`. Each method folder holds the code,
its `tests/`, and where it has an entry point a `commands/`.

**Thirteen command-line tools.** Three ask how few multiplications a map needs and
disagree about what they can prove: `minimise-rank` (descent), `decide-rank`
(complete), `walk-scheme` (a walk that moves sideways). `decide-rank-by-sat` puts
that question to somebody else's solver and `list-solvers` says which backends
this machine has. `find-at-rank`, `deflate-strictly` and `enumerate-subspaces`
are the fixed-k, tree-refutation and isomorph-free routes. `curve-bounds` answers
a different question, bounding the rank from a curve's points rather than
searching. Then `sparsify-operator` for the other strand, and `make-tensor` to
build a map to run any of them on. `decide-rank-by-pencil` searches for
nothing, and `factor-over-canonical-basis` returns the rank as the factorisation
it is, with a receipt anybody can multiply out.

Every paper any of it implements is named once, in
[`references.md`](references.md), and the code cites a key.

## Building

Needs a C++20 compiler, CMake ≥ 3.22 and **Givaro** (`sudo apt install
libgivaro-dev`). Givaro is the only build dependency; every solver is optional
and located on `PATH` at run time.

```sh
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build            # everything, about two minutes
ctest --test-dir build -LE slow   # skip the expensive searches
```

`ccache` is picked up when installed and ignored when not, so it shortens a
rebuild without becoming a second dependency. A pinned environment for
reproducing a published number is in [`Containerfile`](Containerfile).

## Citing

[`CITATION.cff`](CITATION.cff). Licence: MIT, see [`LICENSE`](LICENSE) and
[`NOTICE`](NOTICE) for the scope and for credit on material that is not mine.
