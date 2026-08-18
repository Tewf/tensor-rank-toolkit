# The choices, and the reasoning that measurement overturned

Three things here could have gone more than one way: which backend decides
`GF(p)`, which solver runs, and whether the symmetries are broken. Each was
settled by running both, and in two of the three cases the measurement
contradicted an argument that sounded right. The numbers behind them are
[`measurements.md`](measurements.md); how the encodings work is
[`method.md`](method.md).

## Which GF(p) backend survives

**The one-hot CNF encoder, and this time on merit.** Ubuntu's `cvc5` 1.1.2 is
built without CoCoALib and cannot run its finite-field solver at all, but the
upstream 1.3.4 GPL build can, so the comparison the two backends were built for
actually happened. Ground truth from the exhaustive search: `GF(9)` and F₃
2×2-term both rank 3, F₃ 2×3-term rank 4.

| Question | one-hot CNF | cvc5 finite fields |
|---|---|---|
| `GF(9)` find 3 | **0.010 s** | 5.44 s |
| `GF(9)` rule out 2 | **0.008 s** | 0.085 s |
| F₃ 2×2 find 3 | **0.014 s** | 3.00 s |
| F₃ 2×2 rule out 2 | **0.011 s** | 0.022 s |
| F₃ 2×3 find 4 | **0.051 s** | no answer in 150 s |
| F₃ 2×3 rule out 3 | **0.099 s** | 2.22 s |

**Every verdict they both produced agrees**, and agrees with the exhaustive
search. That is what the second backend was for, and it did its job: the
hand-written multiplication table, addition chain and one-hot constraints are
corroborated by an encoding that shares none of them.

So `cvc5` stays, demoted to exactly that role. It is not dead code and it is not
a rival; it is the independent check on arithmetic that would otherwise be
mine alone, and it is reachable with `--backend smt`. Neither backend settles
F₃ 3×6 at its known rank of ten within five minutes.

## Two defaults that were wrong

**Symmetry breaking off.** Ruling out six products for `⟨2,2,2⟩` goes from 24.7 s
to **0.31 s** with the ordering on, seventy-nine times; under cryptominisat, from
*no answer in 120 s* to 1.57 s, which is where "at least seventy-six times" came
from and is a lower bound rather than a ratio. It ships off because an over-strong
constraint would turn a satisfiable instance into UNSAT, which is a wrong lower
bound; it was checked first against all six fixtures of known rank, and every
one is still found. Use it for any question expected to answer no.

It is implemented for the one-hot GF(p) encoding now, and over `GF(p)` it breaks
a second symmetry that GF(2) does not have: `(λa) ⊗ (μb) ⊗ (νc)` is the same
term as `a ⊗ b ⊗ c` whenever `λμν = 1`, so the operand vectors are normalised to
a first nonzero entry of 1. Sound on all three GF(3) fixtures of known rank,
which are still found, and their `k−1` questions still answer no.

**It does not rescue F₃ 3×6**, which still does not answer in 300 s at its known
rank of ten. This paragraph previously guessed that the missing constraint was
the reason. It was not, and why that tensor is hard is open. It is the largest
GF(p) instance here by some way, at 10 122 variables against 216 for the next
one down, so size alone may be the whole answer.

**CryptoMiniSat preferred.** A GF(2) tensor equation is a parity constraint, so
a solver taking it as one line rather than four clauses ought to win. On these
instances native XOR is worth **nothing measurable**: 1.559 s against 1.563 s on
the same question. Kissat, which cannot read an XOR clause at all, is worth
**five times**: 0.31 s on that question, and 34.2 s against 167.9 s on the next
one up. The reasoning was sound and the measurement disagreed, so Kissat is now
tried first.

## A fourth choice, and the only one measurement should not have settled

**Which order the questions are asked in**, which sits outside the three above
for a reason worth naming. **One schedule is implemented and five were priced**,
which [`search.md`](search.md) shows is the same thing here: a question's cost
does not depend on the order it is reached in, so pricing every question prices
every schedule over them, the four nobody wrote included. It barely matters. The
two mandatory questions are 108.461 s of a 110 to 114 s search on GF(16), so the
whole choice is worth about 3%, and the fastest schedule beats the shipped
default by 2.2%.

**The three above were settled by running both because no paper answered them.**
This one had an answer already. `[morgado2013]` named all five of these schedules
and priced their oracle calls a decade earlier, so it was settled by pricing five
because nobody looked first.

What it did **not** say is that bisection loses in practice: its own assessment
puts BIN ahead of linear UNSAT-SAT, 261 solved against 185. That verdict is
`[heras2011]`'s, about core-guided binary search. So our result is a contrast with
the survey rather than a repetition of it, and the reason is ours to give: our cost
range is a dozen values wide, too narrow for the asymptotics to act on. The full
argument is in [`search-in-the-literature.md`](search-in-the-literature.md).

Where the rank sits between the bounds, and the prices: [`search.md`](search.md).
What the field had settled: [`search-in-the-literature.md`](search-in-the-literature.md).
