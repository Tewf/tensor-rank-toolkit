# What it computes

Nine strands, each with its own folder, its own tests and its own README. This
page is the long form; [`README.md`](README.md) carries the headline numbers in a
table and links here rather than restating them, which is the repository's
zero-redundancy rule applied to itself.

**[Rank by descent](descent_search/)**, the cheap direction. Three steps: an
exact matroid greedy for the starting basis, then two relaxations that trade the
guarantee for reach.

| Map | Naive | Reached | Published rank |
|---|---|---|---|
| F2 5×5 | 25 | **14** | 13, `[bdez2012]` |
| F2 3×8 | 24 | **15** | no solution at 14 |
| F2 4×7 | 28 | **16** | no solution at 14 |
| F3 3×6 | 18 | **10** | 10, `[bdez2012]` |

The guarantees are proved rather than asserted, in
[`article/bilinear-rank.pdf`](article/bilinear-rank.pdf): step 1 is exact by
Rado-Edmonds, the descent is sound and terminates, its fixed point is locally
optimal against the whole candidate pool and not merely the part it scanned, and
the orbit quotient is invariant under any subgroup of the stabiliser. Which of
those a test would catch is recorded in
[`descent_search/correctness.md`](descent_search/correctness.md).

**[Rank by exhaustion](exhaustive_search/)**, the expensive direction, which
proves things. It settles small maps outright, reproducing Karatsuba's 3, the
classical 3 and 6 for GF(4) and GF(8), and **rank ⟨2,2,2⟩ = 7** decided from the
tensor in half a second. On F2 5×5 it rules out 9, 10 and 11 products
exhaustively, so with the descent's 14 this proves **12 ≤ rank ≤ 14** here;
`[bdez2012]` report 13. On F3 3×6 both sides are proved in about 25 seconds.

**[Lower bounds without a search](linear_algebra/tensor_rank_sum.h).** Two
rank-sum bounds return a floor from the tensor alone in milliseconds, and they
are tight often enough to remove the dearest question in a sweep entirely: they
raise GF(16) from 4 to **8** and cyclic convolution from 5 to **9**, each of
which previously cost a minute of exhaustion.

**[Sparsifying the operators](matrix_sparsification/)**, which is the other half
of the cost. Strassen's encoding operators go from **12 nonzeros to 10**, and an
alternative-basis operator from **21 to 10**, in milliseconds. Fewer nonzeros
means fewer additions, the cost the multiplication count does not capture.

**[The rank question as satisfiability](satisfiability/).** Håstad proved
deciding tensor rank NP-complete over every finite field, and that cuts both
ways: `formula_to_tensor` turns 3SAT into a tensor, and three encoders turn the
rank question into one a solver answers. A refutation can be written as DRAT and
checked by `drat-trim`, so a lower bound from a solver is verifiable rather than
trusted.

**[Quotienting by symmetry](orbit_reduction/).** A change of coordinates fixing
the target subspace maps solutions to solutions, so one member of each orbit
suffices: **28× on a refutation**, and the ⟨3,3,3⟩ candidate pool collapses from
261 121 to **13 orbits**.

**[Isomorph-free enumeration](oracle_guided_search/).**
`enumerate-subspaces --canonical` is `[mckay1998]`'s canonical augmentation,
which deduplicates with no memory at all. It returns ⟨2,2,2⟩'s 36 solution
subspaces as the **1 orbit** they are, visiting **1982× fewer nodes**. Wall clock
improves only 1.6×, because finding a canonical code by walking the whole group
spends most of the saving on itself, and that is measured rather than glossed
([`deduplication-cost.md`](oracle_guided_search/deduplication-cost.md)).

**[Two slices, without a search](pencil_rank/).** A tensor with two slices is a
matrix pencil, and Kronecker's theory gives its minimal indices and elementary
divisors by exact linear algebra in **polynomial time, with no candidate pool**.
What it will not give is a rank: Ja'Ja's formula is a theorem over an
algebraically closed field, and on `(I_4, C)` over GF(2) it says 5 where the
exhaustive search **proves** 6. So the module reports a proved lower bound, a
sharper count marked provisional, and *exact* only where the pencil is
diagonalisable over the field. Twelve pencils settled by exhaustion, three of
which the classical formula gets wrong, are tabulated in
[`pencil_rank/README.md`](pencil_rank/README.md).
