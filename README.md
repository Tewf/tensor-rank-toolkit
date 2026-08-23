# tensor-rank-toolkit

[![CI](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/Tewf/tensor-rank-toolkit/actions/workflows/ci.yml)
[![Live pages](https://img.shields.io/badge/pages-tewf.github.io%2Ftensor--rank--toolkit-1f6feb)](https://tewf.github.io/tensor-rank-toolkit/)
[![Licence](https://img.shields.io/badge/licence-MIT-lightgrey)](LICENSE)

> [Lire en français](README.fr.md)

**Exact tensor and bilinear rank over finite fields.** The rank of a bilinear
map is how many multiplications it needs. Strassen's seven-instead-of-eight for
2×2 matrices is where fast matrix multiplication comes from, and finding such
decompositions in general is open.

It attacks that from ten directions, from a cheap descent to a solver to a
canonical form that needs no search at all. **Nothing here is ever a float**, so
a reported rank is a fact about the map rather than an artefact of rounding, and
every count below is asserted by the test suite. Timings are not and are not
claimed to be: [`MEASURING.md`](MEASURING.md) has the protocol,
[`reproduce/`](reproduce/) the driver that regenerates them.

## What it computes

Ten strands. Method and caveats: [`what-it-computes.md`](what-it-computes.md).

| Strand | Asks | Headline |
|---|---|---|
| [descent](descent_search/) | rank from above, cheaply | F2 5x5 to **14**, F3 3x6 to **10** |
| [exhaustion](exhaustive_search/) | rank outright, with a proof | **rank of 2x2 matmul = 7**: 7 found and checked, 6 refuted |
| [incumbent](incumbent_search/) | the same tree, cut by what is built | cyclic F2 7 from 15 to **13**, in 22 nodes |
| [rank sums](linear_algebra/tensor_rank_sum.h) | a floor with no search | GF(16) from 4 to **8**, in milliseconds |
| [pencils](pencil_rank/) | two slices, in polynomial time | the Kronecker form, and where Ja'Ja' stops holding |
| [factorisation](canonical_factorisation/) | the rank as `S = C A` | an answer with a receipt anybody can multiply out |
| [satisfiability](satisfiability/) | the same question, to a solver | pool-free, and a refutation checkable as DRAT |
| [symmetry](orbit_reduction/) | one member per orbit | **39.2x fewer nodes** on a refutation, 261 121 maps to **13 orbits** |
| [isomorph-free](oracle_guided_search/) | each class exactly once, no memory | **22 778x fewer nodes** on 2x2 matmul |
| [sparsification](matrix_sparsification/) | fewer additions, rank fixed | a rank-23 ⟨3,3,3⟩ scheme **221 nonzeros to 128**, against 167 for PLinOpt, proved minimal |

**The leaf is where an exhaustive search lives**, and neither of its two routes
forms an element any more: the walk steps in reflected Gray order over GF(2) and
GF(p) alike, **2.52x an element over GF(3)** with the dimension term gone rather
than reduced, and the pool scan carries a residual. Same verdicts, same node
counts, and one consumer card priced against both: [`gpu_leaf/`](gpu_leaf/).

## The finding worth stating on its own

**The expensive step is priced badly.** Step 3 of the descent enumerates the full
pool of rank-one maps. Across the four polynomial fixtures it improved the answer
in **two of four cases**, by one product each, and cost between **36 and 189
times** the first two steps together. A continuation that only makes step 3
faster optimises the part that mostly does not pay:
[`fixtures/README.md`](fixtures/README.md) exists to hold that finding still.

## One pipeline

The rank search recovers ⟨L, R, P⟩; sparsification is what they are for.

```sh
minimise-rank fixtures/f2_5x5.tensor --emit-operators out   # 25 -> 14 products
sparsify-operator out_L.sms                                 # 31 -> 27 nonzeros
```

The browser console runs those two lines in order, once per operator, as a flow:
[`web_interface/`](web_interface/).

## Reading and writing what the field publishes

A ⟨L, R, P⟩ triple in SMS is what the field publishes a bilinear algorithm as,
and very nearly the only thing it publishes, so that is the way in as well as the
way out. Two sources hand them out in quantity: the
[FMM catalogue](https://fmm.univ-lille.fr/), thousands of decompositions listed
by rank, and [PLinOpt](https://github.com/jgdumas/plinopt), a C++ library for
linear and bilinear straight-line programs whose `data/` ships Strassen,
Winograd, Karatsuba, Toom-3 and matrix multiplication up to 32x32x32.

Reading one is a test and not a claim: a Strassen triple published elsewhere
rebuilds the fixture this repository writes from the definition of the map, entry
for entry, and a disagreement would be ours to explain. **None of it is a
dependency**: nothing here links against any of those tools and the whole suite
passes on a machine where none is installed.

```sh
operators-to-tensor L.sms R.sms P.sms -q 2 > map.tensor     # a published algorithm, read in
PMchecker out_L.sms out_R.sms out_P.sms -q 2                # ours, checked elsewhere
```

What to install, both directions and the differences that bite, on one page:
[`formats/interchange/exchanging-files.md`](formats/interchange/exchanging-files.md).

## What is where

Thirteen modules — the directories that own a question, which is every
`add_subdirectory` except the eight that carry no question of their own (`cli`,
`testing`, `run_limits`, `linear_algebra`, `formats`, `map_construction`,
`search_plan`, `gpu_leaf`) — and **thirteen tools**, in
[`what-is-where.md`](what-is-where.md),
with which tool answers which question. The one question each answers that no
other does, and why thirteen rather than eight:
[`OPTIONS/one-question-per-command.md`](OPTIONS/one-question-per-command.md).
Every flag, its default, the measurement that chose it and the recipes people
actually type: [`OPTIONS.md`](OPTIONS.md). Twelve of the thirteen can be driven
from a browser instead, on Python 3's standard library and nothing else:
[`web_interface/`](web_interface/). Every paper any of it implements is named
once, in [`references.md`](references.md), by the key the code cites.

## Two branches

`main` is what won. **`rejected-experiments` is what lost, kept whole**: the
measurement that decided each rejection and the implementation it retired,
because a rejection whose evidence was deleted is indistinguishable from a whim.
On it are the orbit walk the canonical image replaced, the quotient by default
that a find pays 7.4x for, `[beniamini2020]`'s two exact sparsification oracles
with the row-basis heuristic, and `find-at-rank` with its descending sweep.
Nothing there is broken and nothing there is maintained. The index of all of it,
with the number that retired each:
[`retired/README.md`](https://github.com/Tewf/tensor-rank-toolkit/blob/rejected-experiments/retired/README.md).

## Building

Needs a C++20 compiler, CMake ≥ 3.22, **Givaro** and **Boost's headers**
(`sudo apt install libgivaro-dev libboost-dev`). Those two are the only build
dependencies — Boost is needed by [`vendor/permlib/`](vendor/permlib/) alone, for
`boost::next` and `boost::shared_ptr`, and no header outside that vendored
library includes it. Every solver is optional and located on `PATH` at run time.
`ccache` is used when installed and ignored when not, and
[`Containerfile`](Containerfile) pins an environment for reproducing a published
number.

```sh
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build            # everything, about two minutes
ctest --test-dir build -LE slow   # skip the expensive searches
```

Add `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` and symlink the result to the top of
the tree (`ln -sf build/compile_commands.json .`) to give clangd — and any
editor or agent that speaks to it — the real flags. Without it, every module's
headers look missing, because each one owns its own include directory.

## Citing

[`CITATION.cff`](CITATION.cff). Licence: MIT; [`LICENSE`](LICENSE) and
[`NOTICE`](NOTICE) give the scope and credit what is not mine.
