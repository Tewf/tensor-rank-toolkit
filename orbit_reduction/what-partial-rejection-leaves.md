# What partial rejection leaves, counted

`--orbit-test generators` swaps the orbit walk in
[`isomorph_rejection.h`](isomorph_rejection.h) for `|residual|` lookups: a
candidate is struck out only when one surviving generator, applied once, lands it
earlier in the live suffix. Cheaper per candidate, and it lets through branches
the exact rule collapses. So the question is what that duplication costs and
whether the saving buys it back.

That is the trade the literature already frames. Complete symmetry breaking is
barrier-hard, `[anders2024, Thm. 1.1]`, so nobody escapes partial by being
cleverer; partial breaking is what practical systems use, and `[katsirelos2010]`
is the paper that insists the residual duplication be **counted** rather than
assumed small. Keys in [`../references.md`](../references.md).

## The counts

`decide-rank <fixture> --target k -s matmul <n> <m> <k>`, one worker, both rules
on the same question. **No timing is quoted on this page**: a node count
reproduces anywhere and a second on this machine does not.

| question | `full` | `generators` | duplication |
|---|---|---|---|
| `matmul_2x2x2` refuting k = 6 | **648** | 3 307 | **5.10x** |
| `matmul_2x2x2` finding k = 7 | **3 167** | 3 381 | 1.07x |
| `matmul_2x2x3` refuting k = 9, its floor | **1 149 814** | 20 652 725 | **17.96x** |
| `matmul_2x2x3` finding k = 11, its rank | **17 569 116** | 19 072 942 | 1.09x |

The verdicts agree on all four, which is what
[`tests/test_generator_rejection.cpp`](tests/test_generator_rejection.cpp)
asserts and what the containment argument in
[`isomorph_rejection.h`](isomorph_rejection.h) predicts: the cheap walk's tree
**contains** the exact walk's, node for node, so it cannot miss what the exact
one finds and cannot answer a question differently.

## The answer is no, and the reason is on the neighbouring page

**The duplication is 5x to 18x on a refutation**, and a refutation is the case
the quotient exists for: it walks the whole tree, so every orbit not collapsed is
a subtree entered. What the cheap rule has to buy that back with is the per-node
surcharge of being exact, and
[`what-the-quotient-costs.md`](what-the-quotient-costs.md) already measured that
at **1.10x to 1.41x a node**. Removing all of it saves under a third of a node
against paying five to eighteen times as many nodes. The trade is not close, and
no new timing is needed to say so.

`least_in_orbit` is cheap for a reason the table makes visible. It early-exits
the moment it meets a smaller index in the live suffix, and the orbits it walks
are orbits *inside a suffix*, which shrink as `from` advances. At most nodes it is
not paying for an orbit at all; it is paying for a few lookups and a hit. There
is very little to save and a great deal to lose.

**The satisfiable rows sit near 1x, and that is not encouragement.** A satisfiable
search stops at its first witness, so the two rules agree until the first place
they diverge and the walk ends shortly after. Those rows say the duplication had
no room to compound, not that it is small.

## Where the comparison could not be made

`gf16_multiplication` is a 4×4 map over GF(2) and is not a matrix multiplication
tensor, so `--symmetry auto` refuses it — it would have to build 4.06×10⁸
automorphism pairs — and the closed form does not apply. There is no quotiented
run there under either rule, so the fixture is in no row above. That is the
pre-existing limit [`README.md`](README.md) records under "Where it stops",
reached again from a new direction.

## What this does not say

Nothing here says partial rejection is a bad idea in general. `[katsirelos2010]`
finds the partial predicates it studies *"often effective in practice"* while
leaving *"a large number of symmetric solutions in the worst case"*, and its
other finding is that a stronger break is not thereby a slower one — neither of
which this page contradicts.

What it says is narrower and is about this search: **the exact rule here is
already nearly free**, so the cheap alternative has nothing to win and loses a
factor of up to eighteen where it matters. `full` stays the default, which is
also what every node count recorded in this repository was taken with, and
`--orbit-test` exists so that the sentence above is a measurement anyone can
repeat rather than a remark.
