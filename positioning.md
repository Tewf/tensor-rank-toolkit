# Where this repository sits

Written after a proper literature review rather than before one, which is the
wrong order and is why this file exists. Its job is to keep the repository's
claims inside what is actually unpublished.

It is not the survey. `state-of-the-art.md` on `main` maps the field; this file
takes a position inside that map and says which half of it is ours to claim. The
two were briefly the same filename on two branches, which is how one path came to
hold two documents.

## What the field calls this

The object is the **matrix multiplication tensor**, or more generally a bilinear
map as an order-three tensor; the question is its **rank**, and a decomposition
is a **canonical polyadic decomposition**. Upper bounds for small formats are
catalogued by Sedoglavic at [fmm.univ-lille.fr](https://fmm.univ-lille.fr/).
Searching "tensor rank by SAT" or "bilinear rank" finds the method, not the
field, and finds almost nothing.

## The record, August 2026

| format | best known rank | by |
|---|---|---|
| `⟨2,2,2⟩` | 7 | Strassen 1969 |
| `⟨3,3,3⟩` | 23 | Laderman 1976 |
| `⟨4,4,4⟩` | 49 over `Z`, 48 over `Q` | AlphaEvolve 2025 |
| `⟨5,5,5⟩` | 93 | Moosbauer and Poole 2025, `[moosbauer2025]` |

The flip graph here reaches **7** at `⟨2,2,2⟩`, matching the record, and **24** at
`⟨3,3,3⟩`, one above a fifty-year-old one. Both verified against the map. Neither
is a contribution to the field; they say the implementation works.

## Already published, and therefore not ours to claim

- **Flip graphs**, `[kauers2023]`. What `flip_graph.h` implements.
- **Flip graphs with symmetry**, `[moosbauer2025]`: the walk restricted to
  schemes admitting a group action. This is what produced 5x5 in 93 and 6x6 in
  153, and it is the method the orbit work here was heading towards.
- **Orbit flip graphs**, `[ikenmeyer2025]`: Strassen's 7 reproved from an order-6
  group action with no calculation. The same intersection of symmetry and flips.
- **Adaptive flip graphs**, `[arai2024]`, and **meta flip graphs**,
  `[kauers2025meta]` and `[perminov2026]`.
- **An open-source C++ flip graph framework**, `[perminov2026]`, MIT licensed,
  bit-level encoding and OpenMP, 680 formats from `⟨2,2,2⟩` to `⟨16,16,16⟩`, GPU
  variant. **This is the baseline any flip graph result here is measured
  against**, and it is far ahead of this one. That is checked, not taken from the
  papers: of its shipped schemes, **647 of the 647 small enough to verify exactly
  satisfy Brent's equations** in exact arithmetic, over 127 formats, including all
  four it claims as new records (`2x4x11` at 70, `3x5x9` at 102, `3x5x10` at 114,
  `3x7x9` at 141). Reading only its `u`, `v`, `w` matrices and ignoring the code
  that produced them.
- **Flip graphs for polynomial multiplication over `Z₂`**, `[chen2025]`. Their
  Theorem 2: the walk reaches minimum rank from the standard representation for
  every degree pair in `{(1,1),(1,2),(1,3),(1,4),(1,5),(2,2),(2,3),(2,4),(3,3)}`,
  optimality proved by a SAT solver. That is this repository's own subject and
  its own division of labour, published eighteen months ago.

## Where this repository actually sits

Not in the flip graph, which is a reimplementation. In the other half:
**flip graphs only ever produce upper bounds.** No walk proves a lower bound, and
the exact search here with its automorphism quotient does; `F2 5x5 = 13` is a
proof, not a scheme. That asymmetry is the honest position.

There is also one open question the authors name themselves. `[chen2025]`
closes by asking for polynomial multiplication over **`Z₃`, `Z₅`, `Z₇`**, and
names the obstacle: over a field bigger than `Z₂` a constant moves freely between
the three factors of a rank-one tensor, so the walk wanders a space it should be
quotienting, and "it is unclear what is the best way of doing this". That freedom
is a group action. This repository already computes orbits under the
RP-automorphism group, and its fixtures are already `GF(3)` and `GF(5)`.

**That is the only place a contribution is available here, and it is a question
asked in print rather than one anybody has answered.**

### First measurement against it

The walk now compares factor *directions* rather than spellings, which is the
scalar quotient `[chen2025]` asks for, and over GF(2) it changes nothing because
the only nonzero scalar is 1. On GF(3):

| `f3_3x6`, naive 18 | products |
|---|---|
| flip graph, 8 seeds x 60 000 flips | 12 |
| `minimise-rank`, three steps, 7 s | **10** |

So the quotient makes the method *work* over GF(p) and does not make it
competitive there. That is a real answer to the open question and not the
hoped-for one: the scalar freedom was not the only thing holding the walk back.
Worth having before building on it.

## What this changes

The next flip graph run is not more seeds at `⟨3,3,3⟩`, which chases a number
Laderman had in 1976 and Perminov's framework passes without effort. It is the
walk quotiented by the scalar action, on `GF(p)` polynomial multiplication, with
`[chen2025]` Theorem 2 as the baseline over `Z₂` and their open question as the
target above it.
