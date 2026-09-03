# The write-up

[`bilinear-rank.pdf`](bilinear-rank.pdf) is the formal statement of what this
library guarantees: the definitions, the theorems, their proofs, the space
argument separating the two formulations, and the negative results.

It is the authoritative version. The repository's own documents state results
and point here for why they hold, so that a proof lives in one place and is
corrected in one place.

```sh
latexmk -pdf bilinear-rank.tex     # needs a TeX distribution with amsart
latexmk -C                         # remove the intermediates
```

Which of the theorems a test would catch if it stopped being true is a property
of this repository rather than of the mathematics, so it is recorded beside the
code, in [`../../methods/bilinear_rank/greedy_heuristic/correctness.md`](../../methods/bilinear_rank/greedy_heuristic/correctness.md).
