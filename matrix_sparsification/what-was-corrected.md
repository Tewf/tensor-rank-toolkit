# What was corrected

The defects this strand had when it arrived, and what each cost. Kept because a
corrected number with no record of the correction reads exactly like a number
that was always right. The methods themselves are in
[`method/README.md`](method/README.md); the results are in
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
