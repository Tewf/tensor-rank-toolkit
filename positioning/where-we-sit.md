# Where the rank strand actually sits, and the question left open to it

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

**That is the only place the rank strand has a contribution available, and it is
a question asked in print rather than one anybody has answered.**

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
