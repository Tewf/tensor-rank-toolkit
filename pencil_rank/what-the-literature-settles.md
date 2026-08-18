# What the literature already settles

Written after reading `[sumi2009]` properly rather than from its abstract. The
measured table is in [`README.md`](README.md); this page is which parts of it the
literature had already answered, and it is more than was assumed.

**The field-size condition this module was written around is not a conjecture. It
is the hypothesis of a theorem, and the counterexample is in the same paper.**
Both were found by reading `[sumi2009]`, which this repository already cited.

Let `k` be the number of invariant polynomials of `A` that do **not** factor into
*distinct* linear factors over `K`. Then

- `[sumi2009, Thm. 3.3]`, from Ja'Ja': `rank_K(E^n; A) <= n + k`, **provided
  `Card(K) >= deg p_1(A)`**;
- `[sumi2009, Thm. 3.5]`, theirs: `rank_K(E^n; A) >= n + k`, with **no hypothesis
  at all**, where Ja'Ja' had the reverse only when `p_1(A)` splits;
- `[sumi2009, Prop. 3.4]`: over `GF(2)` with `A` the companion of `x^3 + x + 1`,
  the rank is at least 5, so the cardinality hypothesis cannot be dropped.

That last one is row three of the table above, which this repository measured at
exactly 5 against a count of 4 and wrote up as a discovery. It is a published
counterexample, and the measurement confirms a theorem rather than finding a gap.

**So the honest position is better than the one this page used to state.** `n + k`
is a proved lower bound over every field, and it is *exact* whenever
`Card(K) >= deg p_1(A)`, which a program can check. Both are stronger than what
is implemented here, which reports the closure value and calls itself exact only
where the pencil is diagonalisable. Implementing `k` and the cardinality test is
the named next step for this module, and it would settle six of the twelve rows
above outright rather than bounding them.

What remains genuinely open is the cost *below* the threshold, where the table's
three short rows live and where neither theorem applies.
