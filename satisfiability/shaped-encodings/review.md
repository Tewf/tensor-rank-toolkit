# What the sources say the shaping actually is (reviewed 2026-09-01)

Primary documents read (PDF extracts), the load-bearing quotes spot-checked verbatim.
Part of [README.md](README.md)'s plan; this file is the review it required.

## Heule-Kauers-Seidl's instances are Brent equations plus four devices

From arXiv:1905.10192 (JSC) section 3 and arXiv:1903.11391 (SAT 2019),
`[heule2019local]`-line; the challenge repo is github.com/marijnheule/matrix-challenges:

1. **zero-or-two**: each parity's `even(...)` is *strengthened* to the sufficient
   condition "zero or two arguments true" (at-most-two plus not-exactly-one) — a
   streamliner, not an equivalence.
2. **Neighborhood fixing**: ~50 % of the alpha/beta/gamma variables instantiated from a
   known solution.
3. **Zeroing**: half of the type-0/1/2 terms (i2!=j1 etc.) randomly set to zero,
   "motivated by the observation that in most of the known solutions, almost all these
   terms are zero" - a streamliner mined from solution statistics.
4. **The pairing**: the 27 type-3 terms (i2=j1, j2=k1, k2=i1) randomly distributed over
   the 23 summands under a hardcoded quota - "19 summands should contain one term each
   and the remaining four summands should contain two terms each" (verbatim).

Two more facts that change our plan: **instance generation was survival-of-the-fastest**
(candidates yalsat could not crack within minutes were discarded and re-randomised - the
portfolio is across instances, not solvers), and **no symmetry breaking is encoded** (de
Groote's group is used only post hoc to classify solutions). Type classification, quotas
and the row/column zero patterns are Z2-and-matmul-specific; strengthening parities,
fixing from known solutions, mining streamliners from solution statistics, and
discard-and-retry generation are generic.

## Streamlining's contract (Gomes-Sellmann line)

CP 2004 (DOI 10.1007/978-3-540-30201-8_22) and Smith-Gomes-Fernandez (IJCAI 2005):
streamliners are constraints "not implied by the original problem", soundness kept,
completeness lost per streamliner and recovered by the next one in the portfolio - which
is exactly the finds-only doctrine of [../las-vegas/](../las-vegas/README.md): a
streamlined no proves nothing, a streamlined find is a find. Discovery is automatable:
lattice search with racing (DOI 10.1007/978-3-319-98334-9_24, AIJ 2023
10.1016/j.artint.2023.103915) and LLM-generated candidates filtered by short runs
(arXiv:2408.10268; CP 2025 10.4230/LIPIcs.CP.2025.36).

## Cutting numbers, measured once

xnfSAT (SAT 2021, 10.1007/978-3-030-80223-3_29): on these length-23 parities CNF
performance "reach[es] peak at 6" (verbatim); the public CNFs were generated at cut 4,
which measured worse, and our `--plain-cnf` 3-cut worst; their pooled (tree) chaining
beats linear; native XNF "outperforms all CNF versions by a huge margin". Nothing newer
than 2021 re-measures cutting for local search (UNVERIFIED beyond 2021 that 6 stays
optimal).

## The 2026 field

AFSAT (arXiv:2606.06641), GaloisSAT (arXiv:2603.28796) and TurboSAT (arXiv:2511.07737)
none run MM-Challenge and none add streamliners; no published result beats xnfSAT/yalsat
on challenge1. Scheme *discovery* records moved off SAT entirely: Kauers-Moosbauer flip
graphs (ISSAC 2023, arXiv:2212.01175; framework arXiv:2603.02398, which reportedly states
exhaustive SAT on (3,3,3;23) "runs for days" where flip graphs found 17 000+ schemes -
reported, not quoted; the PDF was not retained). This repository's own
[flip_graph/](../../flip_graph/) is in that family, which repositions the SAT route:
shaping is how SAT stays relevant for *finding*, and the flip graph is the incumbent to
compare against, not only kissat.
