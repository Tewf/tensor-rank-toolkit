# What the literature already settles

Written after reading `[sumi2009]` properly rather than from its abstract. The
measured table is in [`README.md`](./); this page is which parts of it the
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

**Both are now implemented**, in [`sumi_bound.h`](sumi_bound.h). `invariant_factors`
puts the Smith diagonal into the divisibility chain that `elementary_divisors`
deliberately skips, because `k` depends on how the prime powers are distributed
among the invariant polynomials and not merely on the multiset. The test for
"splits into distinct linear factors over GF(p)" is divisibility by `x^p - x`.

What that bought, on the fixtures:

| fixture | rank | closure alone | with `n + k` |
|---|---|---|---|
| `gf4_multiplication` | 3 | 2 | **3, exact** |
| `w_state` | 3 | 3 | **3, exact** |
| `pencil_split_f3_3` | 3 | 3 | **3, exact** |
| `pencil_nilpotent_f2_3` | 4 | 4 | 4, a bound |
| `pencil_irreducible_f2_4` | 6 | 4 | **5**, a bound |
| `pencil_singular_f2_2x3` | 3 | 3 | does not apply |

Two rows gain a proved exact answer where they had a bound, and two gain a
strictly better bound. Nothing became unsound, which the test asserts separately
from the values: a proved bound above the rank would be a false refutation and
nothing downstream would catch it.

The theorems are about `(E^n; A)`, so the module puts the pencil in that form by
inverting an invertible member. A singular pencil has none, and over a small field
a regular one may have none either, since `det` is a form of degree `n` while the
projective line has `p + 1` points. Those report "does not apply" rather than a
guess, which is the `pencil_singular_f2_2x3` row.

What remains genuinely open is the cost *below* the threshold, where the table's
three short rows live and where neither theorem applies.
