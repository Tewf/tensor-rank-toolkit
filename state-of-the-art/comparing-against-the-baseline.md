# Comparing against the baseline, and what has been run

The claim "state of the art" is earned against somebody else's code on the same
question, not against this repository's previous version. The baseline is
`[yang2025]`, whose implementation is public:
`github.com/coolcomputery/tensor-cpd-search`, MIT, Java and Python.

**Nothing from it is vendored here.** It is cloned outside the tree and run
there; this file records what it said.

## What it offers

| route | language | what it is |
|---|---|---|
| `cpd/skip-axis/` | Java | the current fastest, `arXiv:2502.12390`. `Main` is a hardcoded speedtest, not a CLI: it sweeps `⟨2,2,2⟩` over F2 from `R=0` |
| `cpd/original/cpd_search.py` | Python | the older algorithm, `arXiv:2411.14676`, a real CLI with `--matmul n m k` and `--pruners rref ranksum` |
| `border-cpd/` | Python | border rank over `F[x]/(x^H)`, which this repository does not do |

The Python CLI takes `--matmul 2 2 2` and a target, which is exactly the shape of
`decide-rank <fixture> --target k`, so questions match without a translation.

## First run, `⟨2,2,2⟩` over F2

One core each, this machine. Their counters are theirs and ours are ours; see
the caveat below before reading the middle column as agreement.

| question | `cpd/skip-axis` (Java) | `cpd_search.py` (Python) | `decide-rank` (C++) |
|---|---|---|---|
| refute R = 6 | 25 200, **0.954 s** | 3 376, **30.45 s** | 25 399, **0.0362 s** |
| refute R = 6, quotiented | — | — | **648**, **0.0013 s** |
| find 7 | 10 592, 0.407 s | — | 7 436, 0.0214 s |
| whole sweep | ~1.385 s | — | **0.0554 s** |

**Three readings, and only one of them is about being faster.**

The Java baseline and the plain search here visit **25 200 nodes against 25 399**,
0.8% apart. Two independently written searches agreeing that closely on a tree
size is evidence they are the same algorithm, which is what both papers say they
are. The 25x in wall clock is C++ against Java and a bit-packed leaf against a
general one. **It is an implementation result and should not be reported as an
algorithmic one.**

The orbit quotient is the algorithmic difference, and it is on the same question:
**648 nodes, 39x fewer than the baseline visits.** `[yang2025]` prunes by rank
sums and by rref; `[covanov2019]`'s quotient removes whole orbits. They are
different mechanisms and nothing here shows they cannot be combined.

The Python route is slower than both by three orders of magnitude and visits
**7.5x fewer nodes than either**, which is the pruners doing real work in a
language that cannot cash it.

## The caveat that has to be closed before any of this is quoted

**Their `work` counter and our `nodes` may not count the same event.** Ours
counts a call to `descend`. Theirs is a list per depth and the numbers above are
its sum. Until `CPD_DFS.java` has been read closely enough to say the two count
the same thing, the 0.8% is a coincidence that has not been ruled out, not an
agreement that has been established.

## What is not run yet

- Any question larger than `⟨2,2,2⟩`. `f2_5x5`, `gf16_multiplication` and
  `cyclic_f2_5` all translate directly through the Python CLI's `--shape/--data`.
- `cpd/skip-axis` on anything but its two hardcoded tensors, which needs a small
  driver written against `CPD_DFS.search(T, R)`.
- Anything of order 4 or more, which is where `[yang2025]` leads and this
  repository does not go.
