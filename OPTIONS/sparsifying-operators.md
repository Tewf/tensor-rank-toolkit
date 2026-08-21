# Sparsifying operators

`sparsify-operator` runs every method on one operator and reports what each
reached. **None of them is a default**: the command's output is the comparison,
so nothing here is chosen on the caller's behalf. Precedence and
`BILINEAR_TUNABLES`: [`precedence-and-tunables.md`](precedence-and-tunables.md).

| Flag | Default | What chose the default |
|---|---|---|
| `--show` | off | Nothing to measure: prints the sparsified matrix as well as its count. |
| `--max-memory N` | `2G` | Argument: it leaves room on a 16 GB desktop for a browser and an editor to survive the run. Every method enumerates column subsets, C(b, a-1): 35 for the 7x4 operators shipped, about 1.6e13 for a <4,4,4> one. |

## The methods, and which measurement separates them

| Method | What is measured |
|---|---|
| `row-basis heuristic` | Reaches 10 nonzeros on all three fixtures, in milliseconds. |
| `exact oracle, bottom-up` | Reaches 10. |
| `exact oracle, top-down` | Reaches 10. |
| `greedy, by rescaling` | Reaches 10 nonzeros **and 10 operations**, where both oracles reach 10 nonzeros and **20 operations**, on the alternative-basis operator. |

**Bottom-up against top-down is undecided.** They reach the same count on every
fixture, and the recorded `seconds` covers the whole command rather than one
method, so no timing separates them. The only comparison is complexity-theoretic
and it does not settle the question either: bottom-up is
`Theta(a * C(b, a-1) * (a^4 + a*b))`; top-down is `O(a * 2^b * (a^4 + a*b))`
worst case with an early exit that may fire much sooner. That is an argument.

**The greedy method's win is real and the reported column hides it.** All four
reach 10 nonzeros on the alternative-basis operator, so the nonzero count
separates nothing; what separates them is `nnz + nns`, the cost the article
minimises, where the oracles leave all ten entries as ninths (twenty operations)
and the greedy leaves ten signs (ten). The counts are asserted in
[`../matrix_sparsification/tests/test_sparsify.cpp`](../matrix_sparsification/tests/test_sparsify.cpp)
and tabulated in
[`../matrix_sparsification/README.md`](../matrix_sparsification/README.md).
The command prints nonzeros only, so it shows the four methods tying.

The greedy method is a heuristic bounded by a heuristic: the article's line 4 is
an `argmin` over the whole space solved by MaxSAT and Z3, and what is here is
exact for a smaller claim. It implements the article's objective; it does not
reproduce the article's numbers.
