# Tensor rank and satisfiability are the same problem

Håstad proved it in 1990: deciding the rank of a three-dimensional tensor is
**NP-complete over every finite field**, and NP-hard over the rationals
(`[hastad1990]`; keys are [`../references.md`](../../references.md)). That has two
directions, and this folder runs both.

| | |
|---|---|
| **Rank is at least as hard as SAT** | [`formula_to_tensor.h`](formula_to_tensor.h) turns a 3SAT formula into a tensor of rank `4n + 2m` exactly when it is satisfiable |
| **Rank is no harder than SAT** | the encoders turn `rank(T) ≤ r` into a formula, and a solver answers it |

The second is why this exists, and **the advantage grows with the instance**,
up to twenty-one times against [the exhaustive
search](../bilinear_rank/exhaustive/exhaustive_search.h) on GF(16). Where both finish they
agree, on every fixture, in both directions, which is what makes them worth
keeping together: a disagreement would mean one of them is wrong, and neither
can check a "no" after the fact on its own. Numbers in
[`measurements.md`](measurements.md); the encodings in
[`method/`](method/README.md).

```sh
decide-rank-by-sat evidence/fixtures/matmul_2x2x2.tensor               # rank exactly 7
decide-rank-by-sat evidence/fixtures/matmul_2x2x2.tensor --target 7    # one question
decide-rank-by-sat evidence/fixtures/f2_5x5.tensor --from 9 --to 14    # sweep for the rank
decide-rank-by-sat evidence/fixtures/f3_3x6.tensor --target 10 --emit-cnf out.cnf
```

Needs a solver on `PATH`, or `--emit-cnf` and your own. `kissat` is tried first
and is the one to install; `cryptominisat` and `cadical` also work, and `cvc5`
serves `--backend smt`. Solvers are run as programs rather than linked, so the
build depends on Givaro and nothing else and a machine without one still builds
and passes its tests. That arrangement costs incremental solving, and whether to
keep paying for it is open: [`solver_process.h`](solver_process.h) states the
trade.

## The three encoders

| | Field | What the unknowns are made of |
|---|---|---|
| [`binary_encoding.h`](binary_encoding.h) | GF(2) | Booleans. A literal *is* a field element, so a tensor equation is one parity constraint |
| [`prime_field_encoding.h`](prime_field_encoding.h) | GF(p) | `p` Booleans, exactly one true, with the field's tables written out as clauses |
| [`field_theory_encoding.h`](field_theory_encoding.h) | GF(p) | nothing. cvc5 has a theory of prime fields, so the equations go across as equations |

GF(2) has nothing in it to get wrong. **GF(p) has two backends and the one-hot
encoder won**, which leaves cvc5 in the role it earned: the independent check on
arithmetic that would otherwise be mine alone, reachable with `--backend smt`.
The one-hot encoder's tables and addition chain are hand-written, and an
encoding sharing none of them agreeing on every verdict is the best evidence
available that they are right. Why it won:
[`choices/the-gf-p-backend.md`](choices/the-gf-p-backend.md).

## What is checked, and how

The encodings are validated by turning the question around: a decomposition
already known is the assignment the formula must accept, and altering one tensor
entry must break it. Håstad's Lemma 2 is checked the same way, and neither needs
a solver installed. `test_end_to_end` walks both directions of the theorem in
one go.

Claim by claim, with what each rests on and which are checked rather than
argued: [`correctness.md`](correctness.md).

## Where this stops

A solver's "no" is a lower bound only if it finished, **and only on the solver's
word unless `--proof` is given**, which writes a DRAT refutation and has
`drat-trim` check it. Only kissat writes one here, so under any other solver the
flag is refused rather than obeyed silently and uselessly. Ruling out six products for `⟨2,2,2⟩` produces a 1.1 MB
certificate that verifies in half a second. A refutation that fails to verify
stops the command. Timeouts and memory
kills are reported as a third answer and never folded into "no", because that would
turn giving up into a proof. A solver's "yes" is checked: the model is turned
back into rank-one matrices and recombined, and the command fails if that does
not reproduce the tensor.

What each claim rests on, and which are checked rather than argued:
[`correctness.md`](correctness.md). How the rank is located between the free
bounds, and the four searches measured to decide it: [`bracket/`](bracket/README.md).

Symmetry breaking ships off by default, because an over-strong constraint would
turn a satisfiable instance into UNSAT and a wrong "no" is a wrong lower bound.
**Turn it on for any question expected to answer no**, where it is worth at
least seventy-six times. What it does and does not rescue:
[`choices/two-defaults-that-were-wrong.md`](choices/two-defaults-that-were-wrong.md).

Nothing here decides rank over the rationals, and nothing could: over `ℚ` the
problem is `∃ℚ`-complete and **not known to be decidable**. That the difficulty
depends this sharply on the field, and why "NP-hard" is the wrong shorthand for
it, is [`complexity.md`](complexity.md).
