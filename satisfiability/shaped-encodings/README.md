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

1. **xnfsat on Heule's own instances**: finds on 6 of 10 at a 300 s cap, four of them on
   every one of five seeds (means 0.06 to 24 s), where kissat finds none - the five-seed
   distribution of 2026-09-01, `../las-vegas/results.md`. The target shape.
2. **kissat on our plain encodings**: Strassen-7 in 0.13 s, f2_5x5@14 found and checked at
   390 s at a 600 s cap (reproduced on the fixed parser). The control every shaped variant
   must beat *for local search* without losing to *for CDCL*.
3. **The walk's violated floor** per instance (5-15 on the 2-2-2-2 family): a shaped
   variant succeeds when the floor reaches zero, not when it merely drops.

## The review (closed 2026-09-01)

Done: [review.md](review.md), primary sources read and the load-bearing quotes verified.
What it settled: Heule's shaping is four named devices (zero-or-two strengthening,
~50 % neighborhood fixing, type-0/1/2 zeroing, the type-3 pairing quota 19x1 + 4x2), the
instances were generated survival-of-the-fastest against yalsat, **no symmetry breaking is
encoded**, cut 6 pooled is the best CNF of a length-23 parity (native XNF better still),
no 2026 solver beats the record or adds shaping - and scheme discovery's actual state of
the art is the flip graph, this repository's own `flip_graph/`, which joins kissat as an
incumbent to beat.

## The experiment plan

Moved to [plan.md](plan.md) when the algebra-fed devices joined it. The literature pass it
required — [encoding-knowledge-review.md](encoding-knowledge-review.md), opened 2026-09-02
on Mohamed's instruction — closed the same day: device 10 is unblocked (canonical form as
orbit lex-minimum, not RREF; SMS and prefix-assignment are the precedents), CryptoMiniSat's
in-search Gauss-Jordan is the new experiment 0, no theory ranks cutting numbers, and the
no-symmetry-breaking doctrine for walks is published field consensus.
