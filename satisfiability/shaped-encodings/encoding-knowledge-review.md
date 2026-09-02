# Encoding knowledge into SAT: the review (closed 2026-09-02)

Opened on Mohamed's instruction before any further device, as `next-review.md`; closed the
same day. His position going in: the walk thesis is *walk plus the correct encoding* as one
unit, so every negative so far refutes encodings, not the thesis; and the device-10
canonical-form (RREF) obstacle should not be a real blocker. Both survive contact. Primary
sources read; the load-bearing quotes below were re-extracted verbatim from the fetched
PDFs (Szeider's SMS survey, Gwynne-Kullmann, Aloul-Lynce-Prestwich).

## 1. Dynamic canonicity does subsume the static obstacle — by redefinition

SAT modulo symmetries (Kirchweger-Szeider, CP 2021, 10.4230/LIPIcs.CP.2021.34; TOCL 2024,
10.1145/3670405; survey CEUR Vol-4116) runs the minimality check as a propagator inside an
adapted CaDiCaL over IPASIR-UP (10.4230/LIPIcs.SAT.2023.8): on a partial object it searches
for a permutation certifying that *no extension* is lex-minimal, and learns a blocking
clause. "The method is sound (it never prunes canonical solutions) and complete (it finds
all canonical solutions)" (survey, verified). SMS never carries our proof obligation
because its canonical set is the **lex-minima** — a finite orbit contains its minimum for
free — and the obligation moves into the pruning rule, discharged per clause by a
polynomial-time-checkable permutation certificate. So device 10's obstacle dissolves **iff
the canonical form is defined as an orbit order-minimum**; an RREF form reimports the
static "every orbit has a representative" argument unless it provably coincides with one.
The NP-hard minimality search runs under a cutoff, which costs isomorph-freeness and never
solutions — our finds-only doctrine already absorbs that.

Scope and cost. SMS supports domain-permutation groups with a hand-written check per
object class (graphs, tournaments, matroids: "we must create different minimality check
algorithms for different combinatorial objects", SAT 2022, 10.4230/LIPIcs.SAT.2022.4);
nothing GL-shaped. Our product-permutation factor S_r is SMS-shaped; de Groote's
sandwiching factor is not. No SMS descendant touches tensors, Brent equations, or matrices
under row operations (survey's full application list; 42 citing papers of TOCL scanned).
It is CDCL-only — the walk gets nothing — and an in-process propagator, a new integration
class against this toolkit's process route (`solver_process.h`). The *static* alternative
with the same lex-minimal trick is adaptive prefix-assignment (Junttila-Karppa-Kaski-
Kohonen, JSC 99, 2020, arXiv:1706.08325): McKay-style canonical extension emitting
pairwise-nonisomorphic prefix assignments — cubes — for any group presentable as
vertex-colored-graph automorphisms. That is this repository's orbit-cube architecture
generalised, and it keeps the external-process route. Tool: SMS is MIT,
github.com/markirch/sat-modulo-symmetries.

## 2. The CAS-feeding line never touched tensor rank; three pieces transfer anyway

MathCheck's mechanism is DPLL(T) with the theory solver replaced by a CAS callback that
learns blocking clauses mid-flight (Bright-Kotsireas-Ganesh, CACM 65(7), 10.1145/3500921).
What the CAS injects, across their 24 papers: algebraic filters (Williamson's PSD test),
canonicity / orderly generation (Lam's problem, Kochen-Specker via nauty), verification.
Nothing on matrix multiplication, tensor rank, bilinear complexity, or Brent equations —
the publication list and the CACM survey were checked, and the SC-square proceedings
searched (targeted, not TOC-by-TOC). The community *claims* Heule-Kauers-Seidl as SAT+CAS
(CASCON 2019, arXiv:1907.04408), but there the CAS runs after search — lifting GF(2)→Z,
classification — never mid-flight. Transferable, in cost order: **(a)** CryptoMiniSat's
in-search Gauss-Jordan over detected XORs (BIRD: Soos-Meel, AAAI 2019; Soos-Gocht-Meel,
CAV 2020, 10.1007/978-3-030-53288-8_22) — one line in our solver roster, no integration;
**(b)** Bosphorus ANF-level Gröbner/XL preprocessing of the cubic system before CNF
(DATE 2019, arXiv:1812.04580, github.com/meelgroup/bosphorus) — exists, never run on
Brent; **(c)** a canonicity callback over IPASIR-UP mirroring Lam/Kochen-Specker —
unbuilt for tensors, and it is the dynamic half of device 10 above.

## 3. Propagation-strength theory cannot rank our cuts; the right vocabulary is three-way

The vocabulary exists — propagation-complete (Bordeaux-Marques-Silva, SOFSEM 2012),
unit-refutation-complete (del Val, KR'94), GAC-by-UP (Eén-Sörensson JSAT 2006; Bacchus
CP 2007), with exponential separations (Kučera-Savický, JAIR 69, arXiv:2001.00819) — but
it cannot explain the cut-number result: a single XOR is propagation-complete *however it
is cut* ("applied to a single equation (m = 1) yields a translation in PC",
Gwynne-Kullmann, arXiv:1406.7398, verified), so PC/URC cannot separate cut 3 from cut 6.
What does separate them: **size-width trade-off** (with s auxiliaries a parity needs
Ω(2^(n/(s+1))/n) clauses — Emdin et al., MFCS 2022, 10.4230/LIPIcs.MFCS.2022.47) and the
**flip-distance mechanism** (a cut chain needs r correct flips, probability decaying
exponentially; pooled is O(log r) — the xnfSAT paper's own analysis). At the system level
the impossibility is real: "no GAC-representation of polynomial size for arbitrary S"
(Gwynne-Kullmann Thm 11.2, verified — via monotone span program lower bounds), which is
the theoretical licence for in-solver Gauss and native XNF. **No theory of the optimal
cutting number exists**; xnfSAT itself: "It is not clear whether there is a universally
optimal setting for the cutting number" (4 for model counting per Soos-Meel, 6 from
cryptographic folklore, Bard-Courtois-Jefferson ePrint 2007/024). Bookkeeping gain:
"propagation-dead" is the UP-refutable class, hardness ≤ 1 in Gwynne-Kullmann's JAR 2014
hierarchy — the campaign's cross-term channel now has a literature name.

## 4. The walk side: nothing supersedes native XNF, and our doctrine is field consensus

Post-2021, nobody re-measured cutting for local search and no software SLS successor to
xnfsat exists (its four citations checked; repo dormant). The only XOR-native walkers
since are hardware: WalkSAT-XNF on memristor crossbars (Im et al., Nat. Commun. 17:2922,
2026, arXiv:2504.06476) and SATurn (GLSVLSI 2026); the 2-XNF line (Andraschko-Danner-
Kreuzer, MCS 18:20, 2024, arXiv:2311.00733) is DPLL-based. kissat's own "walk" picks
rephase targets on plain CNF (Cai-Zhang-Fleury-Biere, JAIR 2022) — nothing for parity. So
plan.md's "2026 stochastic frontier" resolves to: the family already listed, nothing to
add. The canon behind the thesis: SLS success tracks solution density (Hoos, IJCAI-99,
citing Clark et al., CP 1996), dependency chains cripple flipping (Prestwich, DAM 2002),
and symmetry breaking's harm is published, verbatim: breaking constraints "transform
symmetric solutions into deep local minima, thus decreasing the solution density and
increasing the number of local minima" (Aloul-Lynce-Prestwich, SymCon 2007, citing
Prestwich-Roli, CPAIOR 2005, verified). Heule encoding no symmetry breaking and our
every-walk-hurt measurement both instantiate it. One published fact runs *against* a local
conclusion: decimation-fed initialization is a standard SLS win (SP-guided decimation;
HyDeci, MaxSAT eval 2022-23), so device 6's "CDCL-side only" cliff is not field-general —
plausibly our CNF chains, not fixing itself, starve the walk. No head-to-head CDCL-vs-SLS
fixing comparison exists on decision SAT; our calibration is currently the only datum.

## What this changes in the plan

1. **Device 10 is unblocked**, as Mohamed expected: define the canonical form as an orbit
   lex-minimum, not RREF. Two shapes with precedent — static cubes (Junttila-style prefix
   assignment; fits `orbit_reduction/` and the process route) first, an SMS/IPASIR-UP
   propagator for frontier-k later if the cubes pay.
2. **Experiment 0 before any new device**: CryptoMiniSat with Gauss-Jordan on the plain
   and shaped encodings — the cheapest injection the field offers, and it prices the
   "give the solver the linear algebra" route against cut-6-pooled and native XNF.
3. **Re-test device 6 under native XNF** (yalsat/xnfsat on `--emit-xnf` with fixing)
   before accepting the CDCL-only cliff.
4. Phrase encoding claims in the three-way vocabulary of §3; retire any "strongest
   propagating encoding" phrasing — it is not well-formed for parities.

UNVERIFIED, carried: Prestwich's Handbook ch. 2 exact SLS wording (paywalled); TOCL 2024
SMS content seen via survey only; SC-square TOCs not read volume-by-volume; Emdin bound
and del Val from abstracts; kissat-lacks-user-propagator from background knowledge.
