# Device 10's RREF form is unsound — the r² quotient does not transfer to SAT

The encoding-knowledge review left device 10 with one owed obligation: fixing the first
products to a canonical RREF position is sound only if every decomposition's orbit, under
the genuine symmetry group, contains a representative in that form. **Discharged 2026-09-03,
negatively, machine-checked in [`../../verify/`](../../verify/).** The r² quotient stays a
property of the exhaustive search's subspace parametrisation; it cannot be a static
constraint on the tuple encoding.

## The counterexample, verified by Z3

The smallest instance is a 2×2×2 tensor over GF(2). Take `T_ce = e0 ⊗ I`: its first
a-slice is the 2×2 identity, its second is zero. Two claims, both `OK`:

- `witness_tensor_is_rank_two` — **sat**: `T_ce` has a rank-2 decomposition
  (`a1 = a2 = (1,0)`, `b1 = c1 = (1,0)`, `b2 = c2 = (0,1)`).
- `no_rref_decomposition_of_witness` — **unsat**: no rank-≤2 decomposition of `T_ce` has
  its first-factor matrix `A = [[a1],[a2]]` in reduced row echelon form.

Together: `T_ce` is rank 2, but the RREF constraint admits none of its decompositions, so
conjoining it turns a satisfiable rank question UNSAT — a false lower bound, the exact
hazard [`../../orbit_reduction/orbit_cube_boundary/soundness.md`](../../orbit_reduction/orbit_cube_boundary/soundness.md)
names. The reason is structural: `T_ce`'s a-support is one-dimensional, so every rank-2
decomposition forces both first factors to `(1,0)`; the first-factor matrix is `[[1,0],[1,0]]`,
rank 1, and its RREF `[[1,0],[0,0]]` has a zero row — a dropped term.

## Why it is not an edge case: r exceeds the first-factor dimension, always

The counterexample is minimal, but the failure is generic to every rank question worth
asking. The first factors of an r-term decomposition are r vectors in the factor space of
dimension d (for `⟨n,m,k⟩`, `d = nm`). A rank question is interesting only when **r > d** —
Strassen `⟨2,2,2⟩` seeks 7 terms with `d = 4`, `⟨3,3,3⟩` seeks 23 with `d = 9`. With `r > d`
the r first factors cannot be linearly independent, so the r×d first-factor matrix has rank
≤ d < r, and its RREF carries at least `r − d` zero rows — at least `r − d` vanishing terms.
Fixing the first factors to RREF therefore cannot express any full r-term decomposition of
these tensors at all. The `2^(r²)` basis freedom the exhaustive search quotients is the
freedom of a **basis of an r-dimensional span**; the tuple encoding's first factors do not
form one, so there is no such freedom to quotient statically.

## What survives, and where the r² actually lives

- **The symmetry quotient is sound and already built.** The orbit cubes pin the first term
  to a representative of its orbit under the tensor's automorphism group and `S_r` — a
  genuine symmetry of the decomposition set — which is sound over GF(2) and is what
  `orbit_reduction/` supplies. That is device 10's symmetry half, and it needs no RREF.
- **`GL_r` is not a symmetry.** A change of basis of the products' span mixes rank-one terms
  linearly, which is not rank-one, so `GL_r` does not act on decompositions. Neither a static
  RREF constraint nor an SMS-style dynamic canonicity propagator can quotient it — a
  propagator quotients a group action, and this is not one.
- **The r² is real only in the subspace formulation.** The exhaustive search enumerates the
  span `V` and asks whether `V` contains r independent rank-one maps (`subspace_walk.h`,
  `exhaustive_search/generating-candidates-from-the-span.md`); there the basis choice is
  absorbed into the leaf, not fixed. Carrying the r² into SAT means encoding *that* question
  — variables for a subspace, a leaf-shaped rank-one-content constraint — a different
  encoding, not a constraint bolted onto the tuple one. Whether that encoding is worth its
  own soundness argument is the open question device 10 becomes.

## Consequence for the plan

Device 10 as written (RREF on the tuple encoding) is closed as unsound. The buildable
shaping work reverts to the campaign's next devices — closure-aware zeroing, reject-at-birth
generation, the SAT 2019 row/column streamliners, occurrence constraints — none of which
touches soundness the way a symmetry constraint does. The r² remains the exhaustive search's
advantage, and the only sound route to it in SAT is the subspace re-encoding, filed here as
a research direction rather than a device.
