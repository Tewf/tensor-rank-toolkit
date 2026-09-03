# Where the SAT route stands — the dominance front

One ordering for the whole SAT strand: which solver wins on which encoding, and which
encoding device is kept versus dominated, each with the number that decides it. The
numbers live in the files this one links; nothing is restated here that a linked file
owns. Opened 2026-09-02 to consolidate the las-vegas and shaped-encodings work, which was
recorded per-module but never sorted into one front. The field-vs-us positioning is a
different document, [`../the-research-front/`](../the-research-front/README.md); this one
is internal, about our own solvers and encodings.

The task is **finding upper bounds**: a decomposition at rank `k` re-multiplied and checked.
Every solver below is priced as a *finder*; a walk's "no" is a timeout, never a bound
([`las-vegas/what-was-built.md`](las-vegas/what-was-built.md)). `kissat` alone also refutes.

## Solvers, sorted by what they dominate

The ranking is not absolute — it **inverts with the encoding**, which is the whole finding
of the strand. Two regimes, from [`las-vegas/results.md`](las-vegas/results.md) and the
2026-09-01 five-seed run:

| regime | dominant → dominated | the deciding numbers |
|---|---|---|
| **plain encoding, easy** (Strassen ⟨2,2,2⟩, the poly fixtures) | `kissat` ≫ every walk | Strassen-7: kissat 0.13 s; yalsat 1/5, xnfsat 0/5, probSAT 0/3 |
| **plain encoding, frontier** (⟨3,3,3⟩@23, f2_5x5@14, cyclic_f2_7@13) | `kissat` — the only one that finds *or* refutes | all walks 0 at 60 s; kissat finds f2_5x5@14 at 390 s; the rest unknown |
| **shaped encoding** (Heule's challenge1) | `xnfsat` ≻ `yalsat` ≻ `kissat` | xnfsat 6/10, yalsat 3/10, **kissat 0/10** (5 seeds, 300 s) |

Among the walks the order is fixed by native-XOR support, on every parity-heavy instance:
**`xnfsat` (native XNF) ≻ `yalsat` (native XOR) ≻ `probSAT` ≈ `multilinear-sat` (CNF only)**.
`multilinear-sat` is dominated wherever priced (0/3 on every non-trivial fixture); its one
open lever is native XOR, unbuilt ([`las-vegas/not-built.md`](las-vegas/not-built.md)).
The batched GPU walk (device 9) is not a standalone finder but a streamliner generator, and
is unpriced.

**`kissat` above the rank owns the dense top of the descent**, corrected 2026-09-02:
on ⟨3,3,3⟩ the walk itself finds k=30 (0.03 s) and k=27 (1.4 s) and goes dark from k=25
([`shaped-encodings/campaign-2026-09-01.md`](shaped-encodings/campaign-2026-09-01.md), the
sweep correction). Density above the rank rescues the walk; the approach to the rank
starves it.

## Encoding devices, kept versus dominated

Kept = it moved a number in the right direction, with the number. Dominated = it was tried
and lost, with the evidence, per the rule that a rejection with its evidence deleted is a
whim. Full records in [`shaped-encodings/campaign-2026-09-01.md`](shaped-encodings/campaign-2026-09-01.md)
and [`shaped-encodings/plan.md`](shaped-encodings/plan.md).

**Kept**
- **Native XNF** — the walk's baseline; hands `xnfsat`/`yalsat`/CMS the parities as `x`
  lines. Without it every walk is dominated by `kissat`.
- **Neighborhood fixing (device 6)** — Laderman at ≥48 % hands `kissat` the 23-scheme
  (0.34 s at 50 %, 2.4 s at 48 %, nothing below 45 %). CDCL-side: the walk gets nothing,
  confirmed 2026-09-02 (50 %, four seeds, 300 s, all dark). The plan's predicted split, seen.
- **Cross-term-channel fix** — redraw pairings so no forced term collides with an
  exclusivity unit; survival at zeroing 25/50 rose 8/210 → 29/60, instances now search
  honestly rather than dying to propagation.

**Dominated, with evidence**
- **Devices 3 + 4 alone** (zero-or-two + pairing) — 250 shaped ⟨3,3,3⟩@23 instances,
  197 propagation-dead, 0 finds among the 53 that searched, up to 900 s.
- **In-encoding symmetry breaking for the walk** — hurts every walk measured here; the
  published reason is that it deletes the solution copies a walk feeds on
  ([`shaped-encodings/encoding-knowledge-review.md`](shaped-encodings/encoding-knowledge-review.md),
  Aloul–Lynce–Prestwich). Kept only for `kissat`, where it is worth ≥76×.
- **Plain 3-cut CNF** — the worst of the eight CNF encodings the xnfSAT paper measured;
  cut-6-pooled and native XNF dominate it.
- **CryptoMiniSat + Gauss, defaults *and* tuned** — dominated by plain `kissat`. At
  defaults it fails ⟨3,3,3⟩@23, f2_5x5@14 and cyclic_f2_7@13 at 600 s on both cut-6-pooled
  CNF and native XNF, where `kissat` finds f2_5x5@14 in 390 s. Tuned — `--maxxormat` lifted
  400 → 100000 so the 729-parity matrix is echelonized, native XNF so Gauss actually holds
  the XORs — it **still fails all three at 600 s** (2026-09-02). On the CNF path Gauss never
  engages at all: this build caps XOR *recovery* at width 8 and our parities are width 23.
  So the linear-algebra route does not reach the frontier; it is orthogonal to the search,
  not a substitute for a better encoding. And on **Heule's ten challenges it finds 0/10**
  at 300 s (2026-09-03, XORs recovered and three Gauss matrices echelonized before it
  searched — a real no-find, verified against the logs), where `xnfsat` finds 6/10. So the
  regime table's shaped row holds against Gauss too: `xnfsat` ≻ everything, including CMS.

## Measurement status — the queue that settled the above, drained

Complete. Experiment 0 landed in both regimes (CMS+Gauss dominated on the fixtures, 0/10 on
the challenges), and the closure-wave survivors a missing binary voided on 2026-09-01 are
re-run: **all 17, across 25 runs at 900 s, every one a no-find** — the honest wave record is
closed with zero finds, as the campaign predicted the unbuilt devices would leave it. Raw
cells in `work/2026-09-01_shaped-encodings/out/2026-09-02-queue2/summary.txt`.

## Owed, not yet built

- **Device 10 — the `2^(r²)` quotient.** The orbit argument (RREF-of-first-factors
  reachable within the true symmetry group) is owed before the encoding is sound; the clean
  form is the `S_r` orbit lex-minimum via cubes. See
  [`shaped-encodings/encoding-knowledge-review.md`](shaped-encodings/encoding-knowledge-review.md).
- **A walk-favouring encoding** for the frontier — every device so far is CDCL-side or
  neutral; the walk still owns only the dense top.
- Closure-aware zeroing, reject-at-birth generation, the SAT 2019 row/column streamliners,
  occurrence constraints — the campaign's next devices, in that order.
