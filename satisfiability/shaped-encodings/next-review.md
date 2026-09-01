# Next review: encoding knowledge into SAT so solving gets faster

Opened 2026-09-02 on Mohamed's instruction, before any further device is built. His
position, recorded: the walk thesis is *walk plus the correct encoding* as one unit, so
every negative so far refutes encodings, not the thesis; and he does not expect the
canonical-form (RREF) obstacle of device 10 to be a real blocker. This review is the
literature pass that settles both before more code.

## The question in one sentence

How does the field inject problem knowledge - algebra, symmetry, implied structure -
into a SAT encoding or solver so that solving gets faster, which of it applies to
rank-at-most-k over GF(2), and which of it is already built somewhere.

## Search anchors (names to verify, not claims)

- **SAT modulo symmetries (SMS)** - Kirchweger and Szeider's line: canonicity enforced
  *dynamically* by a propagator during solving instead of statically encoded, which is
  exactly the shape of the device-10 obstacle (a rank-one basis cannot be forced into
  RREF statically; a dynamic canonicity check may not care). If SMS or its descendants
  handle our group (products' permutations x the tensor's symmetries), device 10 may be
  an integration, not a theorem.
- **CAS + SAT / programmatic SAT** - the MathCheck line (Bright, Ganesh, et al.):
  a computer algebra system feeding learned facts into CDCL mid-flight; the established
  "encode the algebra" route, applied to combinatorial algebra conjectures. Also the
  **SC-square** community (satisfiability checking meets symbolic computation), which is
  the venue where this exact question lives.
- **Encoding strength theory**: propagation-complete / unit-refutation-complete
  encodings, generalised arc-consistency of encodings, auxiliary-variable trade-offs,
  extended resolution - the vocabulary for "the same constraint, encoded so the solver
  sees more". Our cut-number result (worst to best measured) is one instance of a
  general theory worth knowing.
- **Preprocessing and inprocessing** as knowledge injection (learned implied clauses,
  symmetry-breaking predicates, cardinality detection): what a modern solver would
  derive from our encoding anyway, versus what must be given.
- **For the walk specifically**: whatever exists on local-search-aware encodings beyond
  the xnfSAT cutting result - the review closed in [review.md](review.md) found the
  2021 measurement and nothing newer; this pass looks again with the new vocabulary.

## What it must settle (finishable criteria)

1. Whether dynamic canonicity (SMS-style) subsumes device 10's static orbit argument,
   and at what integration cost against this toolkit's own solver-process route.
2. Whether the CAS-feeding line has ever touched tensor rank or Brent equations, and
   what it injected.
3. A named, defensible answer to "is there a theory of encodings that predicts our
   cut-6-pooled and shaped-instance measurements", with the baseline the experiments
   would compare against.
4. The walk side: any published encoding transformation that measurably helps
   stochastic local search on parity-heavy formulas, beyond native XNF.

## Standing constraints

The review contract of the las-vegas line applies (verified references, thesis steps);
the arms discipline prices anything that gets built afterwards; and per the sweep of
2026-09-02, the walk's failure is shape-bound at every k, so the walk-favoring encoding
is the strategic target and CDCL-side reductions (device 10) are the frontier-k weapon.
