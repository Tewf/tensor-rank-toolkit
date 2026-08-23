# Where the scan stops, and who already has the answer

The method is [`exact-over-q.md`](exact-over-q.md). This page is the wall it
meets, what was read about getting past it, and what of that survives contact
with `Q`.

## The wall is combinatorial

The scan has an upper bound and **no lower bound**, so it stops only on
collecting `r` vectors. Measured: `4x4x4_49_156_L`, a 16-dimensional space in
`Q^49`, holds 9 of its 16 vectors at weight 4 within 5 s and finds nothing new at
weight 5. The remaining 7 need weight 6 or more, where the scan is `C(49,6)`, 14
million subsets, reaching 451 million at weight 8. Neither the C++ nor an
independent reference finished it in 30 minutes, which is how this was found out.
**That is a fact about the combinatorics, not about the code, so running it
longer buys nothing** — and the command no longer tries: it prices the walk at
1.4 PiB and refuses in milliseconds, naming the number.

The problem this reduces to is the one coding theory has computed for forty
years, minimum-weight codewords of a linear code, and its standard algorithm
**Brouwer-Zimmermann** `[zimmermann1996]` carries a lower bound from several
disjoint information sets and prunes on it. That is the first thing to try here,
because it prunes this same enumeration rather than replacing it. What that
bound is worth is measured rather than hoped: `[hernando2019]` reports 198
million codewords generated against Magma's 6 001 million on one code, so the
saving is the pruning and not a faster inner loop.

**The oracle this scan implements already has a pruned algorithm, in somebody
else's vocabulary.** `[sanjose2025]`, Definition 2.7, defines the relative
generalized Hamming weight

> `M_r(C₁, C₂) = min{ |supp(D)| : dim D = r, D ∩ C₂ = {0} }`

and at `r = 1` that reads `min{ wt(c) : c ∈ C₁ \ C₂ }`, which is
`[beniamini2020]`'s Problem 2.15 with `C₂` the span of what is settled: the
oracle Algorithm 2 calls, word for word. It comes with the full
Brouwer-Zimmermann bound and ships in Sage. **`RGHW(C, C₂, 1)` is the baseline
this method has to be measured against, and this page names it rather than
discovering it later.**

**One word of caution on the word "greedy".** `[chenklove2001]` uses *greedy
weight* for the support weight of a greedy subcode, a union of supports. What
this page calls a greedy weight is the weight of the single vector the matroid
greedy takes at step `i`, and what the module minimises is the *sum* of those.
Three different quantities, one adjective.

**What has to be built here, and it is narrower than it looked.** Nobody has put
the Rado-Edmonds driver on top of a pruned oracle and summed the successive `M₁`
into a minimum-weight basis, and nobody has done any of it outside a finite
field. Both halves exist and never call each other.

**The obstruction over `Q`, stated exactly.** The bound consumes only the
invariant "every support of size `≤ w` is finished", which is field-independent.
What is not field-independent is what a support *holds*: over `F_q` an
information support of size `w` carries `(q−1)^w` vectors and you list them; over
`Q` it carries an infinite family and you must *minimise* over it instead. That
minimisation is cheap where it matters, one projective point at `|S| = 1` and a
pencil with at most `n` candidate ratios at `|S| = 2`, and it is the whole of the
new work.

The minimum-weight basis is also **not unique**, and which one comes back is
decided by the order the subsets are walked in. That does not change the count,
which is what this method promises. It does change what a downstream common
subexpression pass makes of the result, measured in
[`../measured-with-other-tools.md`](../measured-with-other-tools.md).

**Two accelerations that would have made this walk cheaper were priced and
rejected**, because the linear programme made both pointless:
[`accelerations-not-built.md`](accelerations-not-built.md).
