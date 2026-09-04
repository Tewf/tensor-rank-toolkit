# What was corrected

The defects this strand had when it arrived, and what each cost. Kept because a
corrected number with no record of the correction reads exactly like a number
that was always right. The methods themselves are in
[`method/`](method/); the results are in
[`README.md`](README.md).

**Algorithm 2.4 was unreachable.** `sparsifying_…py:268-272` offers the choice
between the two oracles and both arms call `algorithm2_3`. The top-down method
could never be run from the command line.

**Its search falls off the end**, returning `None` into `v, i = algorithm4(u)`.
It does not fire on these operators, so it was latent rather than observed.

**The search objective was computed on doubles.** This implementation counts zeros
exactly by testing field equality, not floating-point equality on raw values.
Previous approaches using `== 0` on raw floats could miscount: on the
alternative-basis operator they see 86 zeros where 144 exist, if rounding is
applied afterwards. When counts are sound but the raw objective is not, the
reported results are misleading.

Everything here is exact rationals, so none of that is possible: a zero is a
zero because it is one.

## And one this session made, on 2026-08-22

**A citation was attached to the wrong paper.** `references.md` credited ACM
TOMS Algorithm 994 with the codeword counts 198 461 377 against Magma's
6 001 753 644 on a `[115, 60, 13]` code. The numbers are right; the paper is not.
They are Table 1 of `[bouyuklieva2021]`, comparing Magma against the authors' own
QextNewEdition. Algorithm 994 is merely reference [11] of that table's paper,
cited there for a parallel implementation.

**How it got in, and why it is worth a paragraph.** The figures came from a
search-result snippet that named Algorithm 994 in the same breath, and the paper
itself is paywalled and returns 403, so nothing checked the pairing. A number
that survives verification while its attribution does not is the most durable
kind of error: the citation lends the number credit, and the number lends the
citation credit, and neither was ever opened. Caught by having somebody read the
primary sources rather than the snippets, which is the only thing that catches it.

The bound itself is now cited to `[lisonek2016]`, read in full, with its equation
number.

## And a second, the same day: a property asserted from a sample

**`4x4x4_49_156_L` was published as having a regular matroid, and it does not.**
A sample of random subsets agreed twice over, and neither agreement was
evidence; a decision procedure refuted it instead. The sample, the witness, and
what the correction leaves standing:
[`method/answering-without-searching.md`](method/answering-without-searching.md).

**What it cost.** One sentence, that the least weight of 4 was minimal *by
Tillmann's theorem*. It was not, and the basis at 100 nonzeros is an upper bound
rather than an optimum. Everything else survived, because it had been checked
rather than deduced: the basis spans the right space, and that was verified in
exact rational arithmetic independently of any solver.
