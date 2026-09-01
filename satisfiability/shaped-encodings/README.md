# Shaping the encoding: making our instances the kind local search solves

Opened 2026-09-01, branch `shaped-encodings`, off `las-vegas-sat` (it needs `--emit-xnf`,
the challenge instances and the measurement driver). Status: **framing and review; no code
until the review below is finished.**

## The problem in one sentence

On Heule's MM-Challenge-1 instances xnfsat finds three of ten in five seconds and kissat
none; on this repository's own encodings of the same kind of question the ranking inverts —
so the difference is the formula, and the question is which of their shaping techniques
transfer to instances we generate.

## What the field calls it

Streamlining (Gomes and Sellmann's term: speculative constraints that keep only a slice of
the solution space); symmetry breaking / term ordering; pairing or anchoring constraints
(fixing part of the first products); XOR cutting (how a long parity becomes CNF clauses —
the cutting number); cube and conquer as shaping-by-splitting.

## What is already known here (measured or reviewed, with the record)

- `--plain-cnf` is the **linear 3-cut, the worst of the eight CNF encodings** the xnfSAT
  paper measured; cutting number 6 was their best, and native XNF avoids cutting entirely
  (`../las-vegas/what-the-field-says.md`, and multilinear-sat's `literature/fft-and-walksat/`).
- The weighted-break rule is **not** the separator: ported into the walk it halves the
  violated floor on every challenge instance and closes none, least of all the instance
  xnfsat solves fastest (multilinear-sat `benchmark/findings-walk/parities.md`, 2026-09-01).
- The repository already has two shaping devices of its own: `--break-symmetry` (the term
  ordering) and the orbit machinery (`orbit_reduction/`), and the two symmetry breaks can
  hold at once over GF(2) — the memory `orbit-cubes-and-term-ordering` says why.
- Heule's instances carry **hardcoded pairings and streamlining** on top of the Brent
  equations; ours carry nothing.

## Baselines (named now, so the review is finishable)

1. **xnfsat on Heule's own instances**: 3 of 10 within 5 s (one seed; tonight's five-seed
   run replaces this number). The target shape.
2. **kissat on our plain encodings**: Strassen-7 in 0.13 s, f2_5x5@14 found at 347 s at a
   600 s cap. The control every shaped variant must beat *for local search* without losing
   to *for CDCL*.
3. **The walk's violated floor** per instance (5-15 on the 2-2-2-2 family): a shaped
   variant succeeds when the floor reaches zero, not when it merely drops.

## The review to finish before any code

- Heule, Kauers, Seidl (the MM-Challenge papers): exactly which streamlining predicates and
  pairing constraints they add, and which are specific to Z2 matrix multiplication.
- Gomes and Sellmann's streamlining line: what survives when the streamliner is wrong
  (completeness is lost per-streamliner, recovered by the portfolio).
- The 2026 record holders the Q2 review named (AFSAT; GaloisSAT/TurboSAT, relaxation feeding
  CDCL): what shaping, if any, they add.
- The cutting-number measurements: whether cut-6 transfers to our parity lengths (23).

## The experiment plan, once the review closes

Generate shaped variants of one fixture family (matmul_3x3x3 at 23 and the 2-2-2-2
challenges as the bridge): plain, cut-6, native XNF, +term ordering, +pairing anchors,
+streamliners from the review — one factor at a time, the arms discipline. Price xnfsat,
yalsat and kissat on every variant with `satisfiability/las-vegas/measure.py --challenges`
(it already prices a directory), five seeds, one protocol. The claim to test: shaping moves
xnfsat from 0 to found on at least one instance our plain encoding loses.
