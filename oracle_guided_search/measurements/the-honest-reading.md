# The honest reading

The commitment applies to two of these five fixtures. A cube is GF(2) only and its
representatives are the closed-form orbits of `⟨n, m, k⟩`, so on `gf16`, `f2_5x5` and
`f3_3x6` the finder is a descending sweep of plain oracle calls with one candidate.
The general orbit route does not rescue them: the ambient group for a 5x5 map over
GF(2) is refused at about `10^14` elements, and a polynomial multiplication tensor's
has about four, which quotients nothing.

No known answer came out wrong. `⟨2,2,2⟩` gives 7 and `gf16` gives 9, both exact, and
`f2_5x5` gives 15, which is a true bound on a rank known to be 12 to 14 and simply a
weak one.

What is worth keeping is not the finder: it is the descending schedule, which hands
`find_rank` a bracket needing one refutation instead of one per rank, the zero-term
drop, which is a correctness fix, and the shared base encoding in `decide_rank`.
