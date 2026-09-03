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
| refute R = 6 | 25 426, **0.954 s** | 3 376, **30.45 s** | 25 399, **0.0362 s** |
| refute R = 6, quotiented | — | — | **648**, **0.0013 s** |
| find 7 | 10 592, 0.407 s | — | 7 436, 0.0214 s |
| whole sweep | ~1.385 s | — | **0.0554 s** |

**Three readings, and only one of them is about being faster.**

The Java baseline and the plain search here visit **25 426 nodes against
25 399**, and the 27 that separate them are accounted for one by one below. Two
independently written searches agreeing that closely on a tree size is evidence
they are the same algorithm, which is what both papers say they are. The 25x in
wall clock is C++ against Java and a bit-packed leaf against a general one.
**It is an implementation result and should not be reported as an algorithmic
one.**

The orbit quotient is the algorithmic difference, and it is on the same question:
**648 nodes, 39x fewer than the baseline visits.** `[yang2025]` prunes by rank
sums and by rref; `[covanov2019]`'s quotient removes whole orbits. They are
different mechanisms and nothing here shows they cannot be combined.

The Python route is slower than both by three orders of magnitude and visits
**7.5x fewer nodes than either**, which is the pruners doing real work that the
implementation cannot convert into wall-clock speed.

## The caveat, closed

The two counters were never known to count the same event, and until they were
this page's middle column was a coincidence nobody had ruled out. They do, the
closing corrected this page's own R = 6 figure from 25 200 to 25 426, and what
survives is 27 nodes traceable to a single line:
[`do-the-counters-agree.md`](do-the-counters-agree.md).

## Past `⟨2,2,2⟩`

Six questions now, over three more tensors, and the baseline is above this search
every time, by 0.06% to 1.04%. Three of the six the baseline cannot finish, which
stopped mattering once its tree turned out to have a closed form; what accounts
for the whole gap is one line of code:
[`the-baseline-prunes-nothing.md`](the-baseline-prunes-nothing.md).

## What is not run yet

- Anything where the baseline would have to *finish* past `⟨2,2,2⟩`. The three
  totals above are the closed form, checked against the shape of tree the program
  demonstrably walks, not observed to the end.
- Anything of order 4 or more, which is where `[yang2025]` leads and this
  repository does not go.
