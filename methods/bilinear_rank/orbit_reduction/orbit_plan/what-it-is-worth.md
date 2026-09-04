# What it is worth, and where to point it

On the polynomial fixtures, `|Stab(T)|` is about 6 over `F₂`, a single-digit
constant at the top of the tree and less below. That turns the seven-hour
`--target 12` into one or two hours. Useful; not a change of kind. And per
[`evidence/benchmark_tensors/decided-exactly.md`](../../../../evidence/benchmark_tensors/decided-exactly.md),
that run now reproduces a published exclusion rather than settling anything.

**Point it at matrix multiplication instead.** ⟨2,2,2⟩ has the sandwich
symmetries and the cyclic one: order in the hundreds over `F₂`, against a
225-element pool. That is where orbits collapse a search rather than trim it,
and [`evidence/benchmark_tensors/`](../../../../evidence/benchmark_tensors/) is where the open
questions are.

## The heuristic is a separate question

Reducing step 3's pool to orbit representatives is one line once the machinery
exists, but it is **not** answer-preserving: `minimise_rank` is
first-improvement with irreversible pruning, so a different pool is a different
walk. It cannot produce a *false* claim, since every result is rebuilt and checked
with `spans_all`, so it is the safe place to experiment. It is not the place
the proof lives. Do the exact search first.

## What threads are worth, and why the split is not at the root

`--threads N` used to be a silent no-op with `--symmetry`: the plain search took
one worker per first choice and the quotiented one took none, so a user asking
for the repository's two best speed-ups got one of them and no warning.
`expand_subspace_up_to_symmetry` now spreads its subtrees too.

**The quotient removes exactly the parallelism the plain search uses**, which is
why the split cannot copy it. The plain search has one first choice per pool
element, 225 at `⟨2,2,2⟩`; here the first choices are one orbit each, which is
five, and collapsing those 225 into 5 is the whole point of being here. So the
frontier is widened a node at a time, breadth first, until it holds at least as
many independent subtrees as there are workers, and the prefix above it is walked
sequentially and charged to the budget exactly as the recursion would charge it.

**On a refutation the node total does not move.** The whole tree is visited
whoever visits it, and [`../tests/test_symmetry_agreement.cpp`](../tests/test_symmetry_agreement.cpp)
asserts the verdict and
the node count at 1, 2, 4, 6 and 12 workers on every fixture it already covered.
**On a satisfiable question the total is an upper bound**, because a witness stops
the search and the subtrees already in flight spend against the same counter: the
finding [`../../exhaustive/what-threads-change.md`](../../exhaustive/what-threads-change.md)
made about the plain search applies here unchanged, and so does its mitigation.
`expand_up_to_impl` tests the shared `found` flag before it consumes a node.

**Measured**, on `decide-rank evidence/fixtures/matmul_2x2x3.tensor --target 9 -s matmul
2 2 3 --node-limit 400000`, which fixes the node count so the rows compare like
for like, under [`../../MEASURING.md`](../../../../MEASURING.md):

| threads | 1 | 2 | 4 | 6 |
|---|---|---|---|---|
| wall | 125.9 s | 65.5 s | 33.3 s | 23.4 s |
| speedup | 1.00x | 1.92x | 3.79x | **5.38x** |

All four visited 400 000 nodes. **Read 5.38x as a floor rather than a figure**:
the package throttled 27 722 times across the four runs, which costs the
many-thread rows and not the one-thread row, so the true ratio is higher and this
chassis will not show it. On the small shapes the difference is invisible for a
different reason: `⟨2,2,2⟩` at target 6 is 648 nodes at every thread count and
15 ms, which is process start rather than search.

**One fan-out, not two.** `decide-rank-by-deflation --parallel` already asks every
candidate at once and each candidate calls this search, so it passes
`spread_over_cores` false and keeps the cores at the outer level, where there is
one branch per candidate and no shared budget.
