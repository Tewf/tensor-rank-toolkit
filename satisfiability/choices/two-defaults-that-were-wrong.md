# Two defaults that were wrong

**Symmetry breaking off.** Ruling out six products for `⟨2,2,2⟩` goes from 24.7 s
to **0.31 s** with the ordering on, seventy-nine times; under cryptominisat, from
*no answer in 120 s* to 1.57 s, which is where "at least seventy-six times" came
from and is a lower bound rather than a ratio. It ships off because an over-strong
constraint would turn a satisfiable instance into UNSAT, which is a wrong lower
bound; it was checked first against all six fixtures of known rank, and every
one is still found. Use it for any question expected to answer no.

| `⟨2,2,2⟩` rule out 6 | kissat | cryptominisat |
|---|---|---|
| Ordering off | 24.7 s | no answer in 120 s |
| Ordering on | 0.31 s | 1.57 s |

For example, on a run here:

```
$ decide-rank-by-sat fixtures/matmul_2x2x2.tensor --target 6 --break-symmetry --plain-cnf
fixtures/matmul_2x2x2.tensor: 4 slices of 4x4 over GF(2)
  lower bound: rank is at least 6
  k = 6 [kissat]: NO, rank is more than 6  (0.335781 s)
```

matching the 0.31 s row above to within run-to-run noise; the full split is
in [`measurements.md`](../measurements.md).

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
