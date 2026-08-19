# What the field already has, and what is therefore not ours to claim

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
  `[kauers2025]` and `[perminov2026]`.
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
