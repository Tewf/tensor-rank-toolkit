# What the program length is, and what it may not be read against

The right-hand column of
[`before-a-subexpression-pass.md`](before-a-subexpression-pass.md) is an
**alternative-basis** count, model (b) crossed with model (c). Three changes of
basis, one per operator, are not charged in it: they are what the sparsification
produced and they are charged separately in the alternative-basis accounting of
`[karstadt2017]` and `[beniamini2020]`, where they cost `O(n² log n)` per
recursion level and so do not touch the leading coefficient. The left-hand column
charges nothing because it changes no basis.

**None of these may be read against the 55 of `[karunaratne2026]`.** That record
is model (c) in the *standard* basis, a straight-line program that makes no basis
change at all and therefore has nothing uncharged. The two count different
things, which is the error
[`../what-it-is-worth.md`](../what-it-is-worth.md) exists to stop repeating.

The temptation is concrete and worth naming, because a reader will do the
arithmetic. On the record scheme itself that table says **52**, and the program
`[plinopt]` ships for it runs in **55**. That is not a record and this page does
not claim one: the 52 leaves three 9×9 changes of basis uncharged and the 55 has
none to charge, and nothing here computes the leading coefficient of an
alternative-basis algorithm with common subexpressions, which is the quantity the
two would have to be compared through. What the 56 in the same row does say is
that `bin/optimizer` at `-O 10000` roughly reproduces its own shipped record, one
addition above it, which is the check that the left-hand column is being measured
properly at all.
