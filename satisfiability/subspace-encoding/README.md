# Encoding the subspace, not the tuple — the sound route to the r² quotient

Opened on branch `subspace-encoding` off `shaped-encodings`. This is a brief to **start a
discussion**, not a device: the design below is a hypothesis with an explicit soundness
obligation to discharge *before* any encoder is written, in the shape that has paid off twice
this line (the encoding-knowledge review, and the device-10 refutation).

## Why this exists

Device 10 asked to spend the exhaustive search's `2^(r²)` saving inside SAT by fixing the
first products to a canonical position. Its RREF form is **refuted** — machine-checked in
[`../../verify/`](../../verify/), written up in
[`../shaped-encodings/device-10-rref-is-unsound.md`](../shaped-encodings/device-10-rref-is-unsound.md):
the r first factors are not a basis of anything (for every real question `r > dim`, so the
first-factor matrix is rank-deficient), `GL_r` is not a symmetry of the decomposition set,
and no static constraint or dynamic propagator can quotient it. So the r² cannot be bolted
onto the tuple encoding.

But the r² is real — it is what makes the exhaustive search beat SAT — and it lives in the
**subspace parametrisation**, where it is sound. The question this branch opens: can that
parametrisation be encoded into SAT, and does it buy the r² there.

## The one insight that separates this from the refuted device

RREF is unsound on the **first-factor matrix** and canonical on a **subspace's basis** — and
those are different objects.

- Device 10 fixed the r×d matrix of first factors to RREF. Those r rows are forced by the
  decomposition and are not free to reduce; RREF drops terms.
- A **subspace** `V` (of the product space where rank-one maps live) has a *unique* reduced
  row echelon basis — this is already how the toolkit names a subspace
  ([`../../orbit_reduction/subspace_canon.h`](../../orbit_reduction/subspace_canon.h):
  the reduced basis, rows by pivot column, is the subspace's canonical code). Enumerating
  subspaces by their RREF basis visits each subspace **once**: the `2^(r²)` basis freedom is
  quotiented by construction, soundly, because here RREF *is* the canonical form of the
  object being searched.

The exhaustive search already searches this way and is the working decider
([`../../exhaustive_search/generating-candidates-from-the-span.md`](../../exhaustive_search/generating-candidates-from-the-span.md),
[`../../exhaustive_search/rank_one_basis.h`](../../exhaustive_search/rank_one_basis.h),
`subspace_walk.h`). The task is to express *its* question in SAT, not to add a constraint to
the tuple encoding.

## The soundness obligation — verify FIRST, before any encoder

The re-encoding is worth building only if it is both **sound** (never turns a satisfiable
rank question UNSAT — the false-lower-bound hazard of
[`../../orbit_reduction/orbit_cube_boundary/soundness.md`](../../orbit_reduction/orbit_cube_boundary/soundness.md))
and **cheaper** (actually spends the r²). The obligation, to be discharged in
[`../../verify/`](../../verify/) on bounded GF(2) cases against the exhaustive decider,
before a line of encoder:

1. **Equivalence.** The SAT encoding of "there is an ≤r-dimensional subspace `V`, given by an
   RREF basis, whose rank-one content expresses `T`" is `sat` **iff** `T` has rank ≤ r.
   Check both directions on small `⟨n,m,k⟩` and on the polynomial fixtures where the
   exhaustive decider gives the ground truth. A one-directional failure is the whole result.
2. **The r² is actually spent.** The assignment count under the RREF-basis encoding is
   `2^(r(d−r))`-shaped, not `2^(r·d)` — confirm the variable count and that no symmetric
   copy of a subspace satisfies the encoding twice (the RREF canonicity is what guarantees
   this; verify it holds as encoded, not just as intended).

Do not skip to the encoder because the insight "feels" sound. Device 10 felt sound and was
refuted by a 2×2×2 tensor; the cost of the check is a morning and the cost of the mistake is
a false lower bound nobody downstream catches.

## Literature to close before building (the review this needs)

As a literature review, name the field's version of this before writing it:

- **Grassmannian / subspace enumeration in SAT and CP** — is "search over r-dim subspaces via
  a canonical (RREF) basis" a named encoding? Check symmetry-free subspace search, matroid and
  q-analog design search (SMS handles matroids — does its subspace machinery apply?).
- **MinRank as SAT** — the leaf ("does `V` contain r independent rank-one maps") is a MinRank
  question (`[buss1999]`, already in `references.md`); the re-encoding is bilinear-rank as an
  outer subspace search over a MinRank leaf. What is the SAT-encoding art for MinRank
  (cryptographic MinRank solvers, the Kipnis-Shamir line)?
- **Flip graphs and the exhaustive engine** as the incumbents to beat — the comparison is not
  only kissat but `flip_graph/` and the exhaustive `decide-rank`, on nodes and on time.

The review is finished when the baseline the encoding would be measured against is named and
runnable.

## The plan, once the review and the soundness check are green

1. Encoding: subspace variables (an r×d RREF basis of `V`, with the pivot-shape constraints
   that make it canonical), plus the leaf constraint that `V`'s rank-one content computes `T`.
2. Price it under the arms discipline against the tuple encoding, `flip_graph/`, and the
   exhaustive `decide-rank`, on nodes and time, on the fixtures the tuple encoding stalls on
   (`⟨3,3,3⟩@23`, `f2_5x5@14`, `cyclic_f2_7@13`).
3. The claim under test: the subspace encoding decides a rank question the tuple encoding
   cannot, at the frontier, by spending the r² — or it does not, priced not argued.

## Open questions to settle in the discussion

- Is the leaf ("`V` contains r rank-one maps computing `T`") itself cheaply SAT-encodable, or
  does it reintroduce a search as costly as the one it removed? The exhaustive search pays
  `p^dim V` for the leaf; a SAT encoder must not hand that cost back.
- Does the RREF-basis encoding interact soundly with the existing orbit cubes and term
  ordering, or does it replace them? (It quotients a different freedom — the basis of `V`, not
  the order or the automorphisms — so in principle they compose, but that is a claim to check,
  not assume.)
- Over GF(p): `subspace_canon` and the RREF canonicity are field-general, but the leaf's
  rank-one test and the normalisation are not (see `../../orbit_reduction/orbit_cube_boundary/`).
