# What each operation costs

The complexity table for [the exact layer](./), and the one caveat that
decides how to read it.

Counted in **field operations**, not bit operations; see the caveat below.
Notation is [the README's](README.md#notation).

| Operation | Time | Space |
|---|---|---|
| `SpanBasis::reduce` | Θ(d·w) |  |
| `SpanBasis::contains` | Θ(d·w) | Θ(w) |
| `SpanBasis::try_add` | Θ(d·w) | Θ(w) added to Θ(d·w) held |
| `rank(A)` | O(r·d·c) | Θ(d·c) |
| `is_rank_one(A)` | O(r·c) | Θ(1) |
| `nonzero_count(A)` | Θ(r·c) | Θ(1) |
| `multiplication_count` | O(k·n·d·m) | Θ(d·m) |
| `flattening_lower_bound` | O(n·m·k·(n+m+k)) | Θ(n·m·k) |
| `contraction(v, T, d)` | Θ(n·m·k) | Θ(n·m·k / n_d) |
| `total_rank_sum_lower_bound_on_axis` | Θ(\|F\|^n_d) given the table | Θ(1) |
| `line_rank_sum_lower_bound_on_axis` | O(\|F\|^(2·n_d)·n_d) given the table | Θ(\|F\|^n_d·n_d) |
| `contraction_ranks` | O(\|F\|^n_d·n·m·k) | Θ(\|F\|^n_d) |
| `spans_all(S, T)` | O((\|S\|+\|T\|)·d·w) | Θ(d·w) |
| `solve_in_row_space` | Θ(e·u²) for `u` unknowns, `e` equations | Θ(e·u) |
| `invert(A)`, A square `c × c` | Θ(c⁴) | Θ(c²) |
| `rank_one_decomposition(A)` | Θ(r·c·d²) | Θ(d·r·c) |
| `multiply(A, B)` | Θ(a·b·c) | Θ(a·c) |
| `transpose(A)` | Θ(r·c) | Θ(r·c) |

## Example

```cpp
#include "linear_algebra.h"

linear_algebra::ModularField field(3);                                // GF(3)
linear_algebra::SpanBasis<linear_algebra::ModularField> basis(field, 6);
const bool added = basis.try_add(candidate);   // candidate: a width-6 vector
```

`try_add` is the `Θ(d·w)` row above, the one a search calls hundreds of millions
of times asking "is this vector new?", and why that row, not any other, is the
one worth having cheap is in the "`SpanBasis` is why the searches finish"
paragraph below.

**The rank sums are the rows above that are not polynomial**, and the only
exponential thing in this layer. They are exponential in the *axis length*, not
in the rank, and the two differ by a whole exponent: the total bound reads the
table once, the line bound enumerates pairs. What that costs in wall clock, and
the one command that measures it, are in
[`tensor_rank_sum.h`](tensor_rank_sum.h) beside the code rather than repeated
here, because a figure kept in two files is a figure that gets corrected in one.
Each bound refuses an axis past its own budget rather than trying, and refusing
only weakens the bound.

**`SpanBasis` is why the searches finish.** Asking "is this vector new?" is the
question they ask most often, and answering it by computing two ranks from
scratch costs Θ(d²·w) each time. Reducing the candidate against a basis already
in echelon form costs Θ(d·w). On F3 3×6 that change alone took step 3 from 29.8
seconds to 11.3.

**`is_rank_one` is the same saving one row further down.** "Is this rank one?" is
asked far more often than "what is its rank?", once per element of every
subspace the exhaustive search walks, and answering it with `rank` pays the `d`
and an allocation per row for a verdict settled at the second nonzero one.
Cross-multiplying against the first nonzero row decides it in one pass and no
inverses; why that is the same question is in
[`measures.h`](measures.h) beside the code.

**`invert` is a factor of `c` off the textbook**, at Θ(c⁴) where a single
Gauss-Jordan on `[A | I]` is Θ(c³): it runs `c` independent solves, one per row
of the inverse. That is deliberate. It reuses the solver that the rank strand
already exercises rather than introducing a second elimination that could
disagree with the first, and the matrices it inverts here are 4×4. If it were
ever asked to invert something large, this is the line to change.

## The caveat that matters: not all field operations cost the same

Over **`GF(p)`** they genuinely are constant time. The primes here are tiny, an
element is one machine integer, and Givaro's modular arithmetic is a multiply
and a reduce.

Over **`Q`** they are not. A rational carries a numerator and a denominator of
arbitrary size, every operation ends in a gcd to keep the fraction reduced, and
elimination makes those integers grow. A field operation on `L`-bit rationals
costs roughly `Θ(M(L) + gcd)` rather than `Θ(1)`, and `L` itself grows through a
Gauss-Jordan pass.

So the table above bounds the *number of operations*, which is the right
quantity for comparing the algorithms, and it understates wall-clock on the
rational side. It stopped being hypothetical on 2026-08-22, when the strand
acquired 23×9 and 49×16 operators: the exact search takes about a third of a
second on the first and refuses the second outright, and the linear programme
that answers it works in exact rationals throughout.
