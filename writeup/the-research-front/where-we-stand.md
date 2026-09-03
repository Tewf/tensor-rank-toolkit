# So where are we, strand by strand

**On the lower-bound side we are close to the front and on it in one place.**
The SAT strand is the same technique as `[heule2021]`, and the measurement that
Kissat beats CryptoMiniSat five times on these instances while native XOR is
worth nothing is not in any paper I could find.

**The sharpest positioning available is `[chen2025]`**, and it is uncomfortable
in the useful way. Chen and Kauers apply the flip graph to *polynomial
multiplication*, which is what every `.tensor` fixture here is, and prove the
schemes optimal **with a SAT solver** (Theorem 7). That is precisely the
division of labour between these two strands, published February 2025. This
repository did not invent the pairing; it rebuilt it, on the same problem
class, without knowing.

**How much of our ground they already cover**, once their degrees are
translated into term counts (`n+m+1` is Toom-Cook, so their `(n,m)` is our
`(n+1)x(m+1)`): their proven-optimal list is 2x2, 2x3, 2x4, 2x5, 2x6, 3x3, 3x4,
3x5 and 4x4 over `Z2`. So **`f2_2x2` and `f2_2x3` are theirs already**, and
`f2_3x8`, `f2_4x7` and `f2_5x5` are not. Their rank is not therefore unknown:
`[bdez2012]` settled `f2_5x5` at 13 and `f3_3x6` at 10 in 2012, and **this
repository now settles `f2_5x5` at 13 by its own two searches**: the exhaustive
one refutes 12 in 146 402 553 nodes and
[`lower-the-bound`](../../methods/bilinear_rank/branch_and_bound/README.md) exhibits 13 in 80. It was
13 ≤ rank ≤ 14 until 2026-08-21, the citation carrying the upper half.
**`f2_4x7` is the one genuinely open map here**, at 15 ≤ rank ≤ 16, their lower
bound against our upper one; deciding 15 would close it and neither side has,
and the incumbent search does not move it either.

They also state the asymmetry this repository is built around, in their own
words: "Flip graphs are useful for finding low-rank tensor representations, but
it is not clear how to use the technique for checking whether an optimum has
been reached."

And their open question is the symmetry both strands attacked today: "constant
factors can be freely moved between the components of a rank-one tensor ... it
is unclear what is the best way of doing this. This may be an explanation why an
automated search in the flip graph works best for `K = Z2`." Two independent
answers were measured here on 2026-08-16 and both are negative: quotienting by
that freedom makes the flip graph run over `GF(3)` without making it
competitive, and breaking the same symmetry in the SAT encoding is sound and
does not rescue `f3_3x6`.

**On the upper-bound side we were a decade behind until today**, when the flip
graph was implemented and recovered Strassen by walking. That is
`[kauers2023]`, the 2023 method, and reaching `[moosbauer2025]` means adding
symmetry to the walk, which is exactly the group the orbit work already computes.

**The one place the upper-bound side is not behind is the polynomial fixtures**,
and it is not because of a better search but because of a better starting point.
A flip walk begins at a decomposition and rewrites it; `lower-the-bound` begins
at the minimum-weight basis of `span(T)`, which is exact and is typically five to
nine products above the answer rather than the naive `n·m`. On `cyclic_f2_7` that
reaches the published 13 in 22 nodes where the descent's step-3 shortlist is 0 of
16 129 and cannot move at all. It is not a rival to a flip graph on matrix
multiplication, where the records are, and it does not claim to be: it is bounded
by `C(|pool|, best − dim span T)` where a walk never runs out of moves.
