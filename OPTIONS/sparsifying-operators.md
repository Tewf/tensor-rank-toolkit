# Sparsifying operators

`sparsify-operator` runs every method on one operator and reports what each
reached. **One of them is now the answer rather than a candidate**: the matroid
greedy over `Q` returns the minimum over every invertible `V`, so no other method
can beat it. The comparison is still what the default output is for. Precedence and
`BILINEAR_TUNABLES`: [`precedence-and-tunables.md`](precedence-and-tunables.md).

| Flag | Default | What chose the default |
|---|---|---|
| `--operations` | off | Argument: the greedy by rescaling minimises `nnz + nns` rather than `nnz`, which is a different question, and it costs about 600x what the answer costs. Asked for, never assumed. |
| `--emit PATH` | off | Nothing to measure: writes that minimum as SMS, the way up the file came in, so another tool can read it. |
| `--show` | off | Nothing to measure: prints the sparsified matrix as well as its count. |
| `--simplex` | off | Argument: it is an upper bound where the default is a proof, so the proof is the default. It is also the only route that answers an operator the search refuses, which is why it is one flag away. |
| `--max-memory N` | derived | Argument: an eighth of what the machine reports, `2G` on the 16 GB laptop every table here was measured on. The scan is priced by what it may *walk*, `C(b, (b−r+1)/2 + 1)`, not by what it allocates: about ten megabytes on a 23×9 operator and 1.4 PiB on a 49×16 one, which is why the second is refused in milliseconds instead of running for half an hour. |

## The routes, and which measurement separates them

| Route | What is measured |
|---|---|
| `exact, matroid greedy over Q` (default) | **The minimum** over every invertible `V`, by Rado-Edmonds. 10 on all three fixtures; 43 / 42 / 43 on the operators of a rank-23 `⟨3,3,3⟩` scheme, in about a third of a second each. |
| `by linear programming` (`--simplex`) | The same counts, four to fifteen times faster on anything large enough to time, and the **only** route that answers `4x4x4_49_156_L`, which the search refuses. An upper bound rather than a proof: see [`../matrix_sparsification/method/answering-without-searching.md`](../matrix_sparsification/method/answering-without-searching.md). |
| `greedy, by rescaling` (`--operations`) | Reaches 10 nonzeros **and 10 operations** on the alternative-basis operator, which is the cost the article minimises. The only route here answering that question, and the reason speed does not order it against the others. |

**Three routes, not five.** The row-basis heuristic and `[beniamini2020]`'s two
oracles reached the same counts 88x to 343x more slowly and moved to the
`dominated-methods` branch on 2026-08-22:
[`../matrix_sparsification/dominated.md`](../matrix_sparsification/dominated.md)
has the measurement and says where to find them.

**The rescaling greedy's win is real and the reported column hides it.** Every
route reaches 10 nonzeros on the alternative-basis operator, so the nonzero count
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
