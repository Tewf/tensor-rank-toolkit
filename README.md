# tensor-rank-toolkit

[![CI](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml)
[![Live pages](https://img.shields.io/badge/pages-tewf.github.io%2Ftensor--rank--toolkit-1f6feb)](https://tewf.github.io/tensor-rank-toolkit/)
[![Licence](https://img.shields.io/badge/licence-MIT-lightgrey)](LICENSE)

> [Lire en français](README.fr.md)

**Exact tensor and bilinear rank over finite fields.** The rank of a bilinear
map is how many multiplications it needs. Strassen's seven-instead-of-eight for
2×2 matrices is where fast matrix multiplication comes from, and finding such
decompositions in general is open.

It attacks that from nine directions, from a cheap descent to a solver to a
canonical form that needs no search at all. **Nothing here is ever a float**, so
a reported rank is a fact about the map rather than an artefact of rounding, and
every count below is asserted by the test suite. Timings are not, and are not
claimed to be: [`MEASURING.md`](MEASURING.md) has the protocol and
[`reproduce/`](reproduce/) the driver that regenerates them.

## What it computes

Nine strands. The numbers below are asserted by the test suite and reproduce
anywhere; the long form of each, with its method and its caveats, is in
[`what-it-computes.md`](what-it-computes.md).

| Strand | Asks | Headline |
|---|---|---|
| [descent](descent_search/) | rank from above, cheaply | F2 5x5 to **14**, F3 3x6 to **10** |
| [exhaustion](exhaustive_search/) | rank outright, with a proof | **rank of 2x2 matmul = 7**, decided in half a second |
| [rank sums](linear_algebra/tensor_rank_sum.h) | a floor with no search | GF(16) from 4 to **8**, in milliseconds |
| [pencils](pencil_rank/) | two slices, in polynomial time | the Kronecker form, and where Ja'Ja' stops holding |
| [factorisation](canonical_factorisation/) | the rank as `S = C A` | an answer with a receipt anybody can multiply out |
| [satisfiability](satisfiability/) | the same question, to a solver | pool-free, and a refutation checkable as DRAT |
| [symmetry](orbit_reduction/) | one member per orbit | **28x** on a refutation, 261 121 maps to **13 orbits** |
| [isomorph-free](oracle_guided_search/) | each class exactly once, no memory | **1982x fewer nodes** on 2x2 matmul |
| [sparsification](matrix_sparsification/) | fewer additions, rank fixed | Strassen's operators **12 nonzeros to 10** |

## The finding worth stating on its own

**The expensive step is priced badly.** Step 3 of the descent enumerates the
full pool of rank-one maps. Across the four polynomial fixtures it improved the
answer in **two of four cases**, by one product each time, and cost between
**58 and 184 times** the first two steps together. Any continuation that only
makes step 3 faster optimises the part that mostly does not pay, and
[`fixtures/README.md`](fixtures/README.md) exists to hold that finding still.

## One pipeline

The rank search recovers the encoding operators ⟨L, R, P⟩ and writes them out;
the sparsification is what they are for.

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out   # 25 -> 14 products
sparsify-operator out_L.sms                                 # 31 -> 27 nonzeros
```

## What is where

Twelve modules and twelve tools, in [`what-is-where.md`](what-is-where.md)
with what each folder serves and which tool answers which question. Every flag,
its default and the measurement that chose it: [`OPTIONS.md`](OPTIONS.md). Every
paper any of it implements is named once, in [`references.md`](references.md),
and the code cites a key.

## Building

Needs a C++20 compiler, CMake ≥ 3.22 and **Givaro** (`sudo apt install
libgivaro-dev`). Givaro is the only build dependency; every solver is optional
and located on `PATH` at run time.

```sh
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build            # everything, about two minutes
ctest --test-dir build -LE slow   # skip the expensive searches
```

`ccache` is used when installed and ignored when not, so it shortens a rebuild
without becoming a second dependency. [`Containerfile`](Containerfile) pins an
environment for reproducing a published number.

## Citing

[`CITATION.cff`](CITATION.cff). Licence: MIT; [`LICENSE`](LICENSE) and
[`NOTICE`](NOTICE) give the scope and credit the material that is not mine.
