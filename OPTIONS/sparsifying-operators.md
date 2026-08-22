# Sparsifying operators

`sparsify-operator` runs every method on one operator and reports what each
reached. **One of them is now the answer rather than a candidate**: the matroid
greedy over `Q` returns the minimum over every invertible `V`, so no other method
can beat it. The comparison is still what the default output is for. Precedence and
`BILINEAR_TUNABLES`: [`precedence-and-tunables.md`](precedence-and-tunables.md).

| Flag | Default | What chose the default |
|---|---|---|
| `--exact` | off | Argument: the matroid greedy over `Q` is proved minimal and is also much the fastest. Off by default because the command exists to compare. |
| `--emit PATH` | off | Nothing to measure: writes that minimum as SMS, the way up the file came in, so another tool can read it. |
| `--show` | off | Nothing to measure: prints the sparsified matrix as well as its count. |
| `--max-memory N` | derived | Argument: an eighth of what the machine reports, which is `2G` on the 16 GB laptop every table here was measured on and moves on its own elsewhere. Every method enumerates column subsets, C(b, a-1): 35 for the 7x4 operators shipped, about 1.6e13 for a <4,4,4> one. |

## The methods, and which measurement separates them

| Method | What is measured |
|---|---|
| `exact, matroid greedy over Q` | **The minimum**, by Rado-Edmonds. Reaches 10 on all three fixtures, and is about a hundred times quicker than any of the others on an operator large enough to time. |
| `row-basis heuristic` | Reaches 10 nonzeros on all three fixtures, in milliseconds. Measured losing to the exact method on 37% of 400 random operators. |
| `exact oracle, bottom-up` | Reaches 10. |
| `exact oracle, top-down` | Reaches 10. |
| `greedy, by rescaling` | Reaches 10 nonzeros **and 10 operations**, where both oracles reach 10 nonzeros and **20 operations**, on the alternative-basis operator. |

**Bottom-up against top-down is undecided on the shipped fixtures.** They reach
the same count on every one, and the recorded `seconds` covers the whole command
rather than one method. The only comparison is complexity-theoretic: bottom-up is
`Theta(a * C(b, a-1) * (a^4 + a*b))`; top-down is `O(a * 2^b * (a^4 + a*b))`
worst case with an early exit that fires much sooner.

Neither is the one to reach for. **Both are beaten on every axis by the matroid
greedy**, which walks the same subsets in the other direction, does not
materialise them, and carries the proof.

**The rescaling greedy's win is real and the reported column hides it.** Every
method reaches 10 nonzeros on the alternative-basis operator, so the nonzero count
separates nothing; what separates them is `nnz + nns`, the cost the article
minimises, where the oracles leave all ten entries as ninths (twenty operations)
and the greedy leaves ten signs (ten). The counts are asserted in
[`../matrix_sparsification/tests/test_sparsify.cpp`](../matrix_sparsification/tests/test_sparsify.cpp)
and tabulated in
[`../matrix_sparsification/README.md`](../matrix_sparsification/README.md).
The command prints nonzeros only, so it shows them tying.

The greedy method is a heuristic bounded by a heuristic: the article's line 4 is
an `argmin` over the whole space solved by MaxSAT and Z3, and what is here is
exact for a smaller claim. It implements the article's objective; it does not
reproduce the article's numbers.
