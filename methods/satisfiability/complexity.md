# "NP-hard" is the wrong shorthand for this problem

Papers on tensor decomposition routinely say tensor rank is NP-hard and stop
there. It is true and it is the least informative true thing available, because
**the difficulty depends entirely on the field**, and NP-hardness is only a
lower bound that flattens the difference.

Schaefer and Štefankovič settled the shape of it (`[schaefer2018]`; keys are
[`../references.md`](../../references.md)): **tensor rank over a field `F` is
polynomial-time equivalent to the existential theory of `F`.** One theorem, and
every case falls out of it.

| Over | Tensor rank is | Which means |
|---|---|---|
| a finite field | **NP-complete** | in NP and NP-hard. Both halves hold |
| `ℂ` | **∃ℂ-complete** | `∃ℂ ⊆ AM`, so inside the second level of the polynomial hierarchy, assuming GRH |
| `ℝ` | **∃ℝ-complete** | `NP ⊆ ∃ℂ ⊆ ∃ℝ ⊆ PSPACE`, and `∃ℝ` appears to hold problems harder than NP |
| `ℚ` | **∃ℚ-complete** | `∃ℚ` is **not known to be decidable**, and is not expected to be |
| `ℤ` (a ring) | undecidable | `∃ℤ` is the halting problem, by Matiyasevich |

Their own summary of the prior state: "Previously all these problems were known
to be NP-hard using Håstad's argument."

## What that costs you if you take the shorthand

**Over `ℚ` there may be no algorithm at all.** Not a slow one: none. A
decidability result for tensor rank over the rationals would imply a decidability
result for `∃ℚ`, which has been open for decades. "NP-hard" suggests a problem
that is merely expensive, and hides that this one is not known to be solvable.

**Over a finite field the sharp statement is stronger, not weaker.**
NP-*complete* says the problem is *in* NP, which NP-hardness alone does not.
That membership is not a technicality here: it is the entire reason this folder
exists. A decomposition is a certificate, so the question can be handed to a SAT
solver. Over `ℝ` you could not do that, because the certificate is real numbers.

## The decision problem and the search problem are different

Both get called "tensor decomposition" and they are not the same object.

| | |
|---|---|
| **Decision** | is `rank(T) ≤ r`? Over a finite field: **NP-complete** |
| **Search** | produce a decomposition into `r` terms, or report there is none. Over a finite field: in **FNP** |

FNP is the function-problem analogue of NP: a witness of polynomial size that a
machine can check in polynomial time. Over `GF(p)` a decomposition is exactly
that, `r(n₁+n₂+n₃)` field elements, checkable by multiplying them out.

**This repository implements that verifier literally.** When
[`rank_question.h`](rank_question.h) gets a satisfying model back it does not
report a rank; it reconstructs the rank-one matrices, recombines them, and
compares against the tensor, and refuses the answer if they differ. That check
is the FNP verifier, and it is why a "yes" here does not depend on trusting the
solver, the encoding, or me.

The asymmetry between the two verdicts follows from the same fact and is worth
being explicit about:

- A **yes** carries its own proof. It is checked, every time, and cannot be
  wrong without the checker being wrong too.
- A **no** carries nothing. It is a claim about everything not visited, so it
  rests on the solver having genuinely finished, which is why "no answer" is
  kept as a third verdict and never folded into "no".

That is also why two independent methods are kept on the same fixtures: for the
`no` answers, agreement between [the exhaustive
search](../bilinear_rank/exhaustive/exhaustive_search.h) and a solver is the only check
available.

## Where the shorthand came from

Håstad proved NP-hardness over `ℚ` and NP-completeness over finite fields in
1990 (`[hastad1990]`). Hillar and Lim adjusted the argument to `ℝ` and `ℂ` and
titled their paper *Most tensor problems are NP-hard*, which is accurate and is
the phrase that travelled. The stronger per-field picture came later, and has
not displaced it.
