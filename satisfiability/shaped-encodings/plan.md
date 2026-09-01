# The experiment plan

Part of [README.md](README.md); the review behind it is [review.md](review.md).
Shaped variants of matmul_3x3x3 at 23 (the bridge to challenge1), one factor at a time
under the arms discipline, priced with `satisfiability/las-vegas/measure.py --challenges`
(five seeds, one protocol), every variant on xnfsat, yalsat AND kissat: the encoding floor
is expected to move only local search, the streamliners could move either, and expectation
is not measurement.

## Replication half (Heule's own devices, from the review)

1. encoding floor: plain 3-cut / cut-6 pooled / native XNF (no shaping);
2. + zero-or-two strengthening (generic, cheapest streamliner);
3. + type-3 pairing under the 19x1 + 4x2 quota, randomised, many instances,
   discard-and-retry after minutes (their generation discipline: the portfolio is
   across instances);
4. + zeroing of type-0/1/2 terms at their observed rate;
5. deliberately no in-encoding symmetry breaking: theirs has none, ours measurably
   hurt every walk.

## Algebra-fed half (what this repository can mine that statistics cannot)

6. **Neighborhood fixing from the toolkit's own objects**: fix a partial product basis
   from a known lower-rank scheme (`descent_search/`, `canonical_factorisation/`) and ask
   SAT for the extension - the same question shape `exhaustive_search/` already walks, so
   the fixed half is algebra, not luck.
7. **Statistics mined from flip-graph output**: `flip_graph/` can generate thousands of
   schemes for a fixture, so the zeroing rates and pairing quotas of device 3 and 4 can be
   *computed* for any tensor rather than copied from matmul folklore - the generalisation
   Heule's matmul-specific term types cannot give.
8. **A sparsity streamliner**: known operators are sparse and `matrix_sparsification/`
   knows how sparse; "at most X nonzeros per row of the encoding operators" is an
   algebra-motivated streamliner not in the literature's list.
9. **GPU-generated streamliners** (the "I am ok with GPU" route): the batched walk's
   low-violation slots vote a consensus assignment; fix the variables the batch agrees on
   and hand the rest to kissat or xnfsat. The named precedent is GaloisSAT/TurboSAT
   (arXiv:2603.28796, 2511.07737): a GPU relaxation injecting confidence-ranked unit
   clauses into CDCL - which is streamlining under another name, and multilinear-sat
   already produces the consensus for free. Their caveat carries over: unsound units lose
   completeness per instance, recovered by the portfolio, so a no still proves nothing.

## The quotient device (Mohamed's, 2026-09-01: the theoretical lever)

10. **Encode the quotient, not the tuple space.** The current encoding searches ordered
    product tuples with full per-product freedom, an assignment space of order
    2^(r(m+n)); the exhaustive search walks *subspaces*, which quotients away the
    ~2^(r^2) bases of the span and drops the exponent to r(m+n-r) - that quotient is
    what made the algebraic search faster than SAT here. The SAT analogue: fix the
    first products in a canonical position (a normal form of the span's basis), which
    is **sound per orbit** when every decomposition's orbit contains a representative
    in that form - a proof obligation, not a heuristic, and `orbit_reduction/` plus the
    memory `orbit-cubes-and-term-ordering` are where that argument lives. Device 6 is
    this device's heuristic shadow: fixing from a known scheme loses completeness,
    fixing a canonical form does not. **Predicted split, priced not assumed**: the
    quotient should help the systematic solvers (fewer symmetric branches) and may hurt
    the walks - symmetric solution copies are food for a walk, which is plausibly why
    Heule encodes no symmetry breaking and why ours measurably hurt every walk. If the
    prediction holds, the portfolio becomes: quotient encoding for kissat, streamlined
    unquotiented encoding for xnfsat - each solver gets the space it wants.

## The claims

Shaping moves xnfsat from 0 to found on at least one instance our plain encoding loses;
and device 9's consensus beats the ascent seed's measured failure (the relaxation's
*rounded point* was a bad start - a *consensus over thousands of walks* is a different
object, and that difference is the experiment). Success is compared against kissat-on-plain
AND against `flip_graph/` on the same tensor, or the win is only against a strawman. No
theoretical speedup is claimed anywhere: nothing in the reviewed literature offers one for
these instances, and every device above is priced, not argued.
