# What it computes

Ten strands, each with its own folder, its own tests and its own README. This
page is the long form; [`README.md`](README.md) carries the headline numbers in a
table and links here rather than restating them, which is the repository's
zero-redundancy rule applied to itself.

**[Rank by descent](methods/bilinear_rank/greedy_heuristic/README.md)**, the cheap direction. Three steps: an
exact matroid greedy for the starting basis, then two relaxations that trade the
guarantee for reach.

| Map | Naive | Reached | Published rank |
|---|---|---|---|
| F2 5×5 | 25 | **14** | 13, `[bdez2012]` |
| F2 3×8 | 24 | **15** | no solution at 14 |
| F2 4×7 | 28 | **16** | no solution at 14 |
| F3 3×6 | 18 | **10** | 10, `[bdez2012]` |

The guarantees are proved rather than asserted, in
[`writeup/article/bilinear-rank.pdf`](writeup/article/bilinear-rank.pdf): step 1 is exact by
Rado-Edmonds (`[oxley, Lem. 1.8.3]` in general, `[nakatsukasa2017, Thm. 2.1]`
for this problem by name), the descent is sound and terminates, its fixed point
is locally optimal against the whole candidate pool and not merely the part it
scanned, and the orbit quotient is invariant under any subgroup of the
stabiliser. Which of those a test would catch is recorded in
[`methods/bilinear_rank/greedy_heuristic/correctness.md`](methods/bilinear_rank/greedy_heuristic/correctness.md).

**[Rank by exhaustion](methods/bilinear_rank/exhaustive/README.md)**, the expensive direction, which
proves things. It settles small maps outright, reproducing Karatsuba's 3, the
classical 3 and 6 for GF(4) and GF(8), and **rank ⟨2,2,2⟩ = 7** decided from the
tensor in half a second. On F2 5×5 it rules out 9, 10 and 11 from the bounds
and 12 by exhaustion, and with the 13 the strand below exhibits that is
**rank(F2 5×5) = 13** proved here on both sides; `[bdez2012]` report the same 13.
On F3 3×6 both sides are proved in about 25 seconds.

**[The same tree, cut by an incumbent](methods/bilinear_rank/branch_and_bound/README.md)**, which is the
upper-bound direction of the one above. A node is a subspace containing
`span(T)` and a child adjoins a rank-one map, exactly as `[bdez2012]` Algorithm 2
does; what changes is that the branch stops at `dim V + 1 >= best` rather than at
a target fixed in advance, so a spent budget hands back an algorithm instead of
nothing. The bound is admissible because `cost(V) >= dim V`, and the direction can
only find and never refute. It moves the two fixtures the descent cannot: **cyclic
convolution of length 7 over F2 from 15 to 13 in 22 nodes**, which is its
published rank, where not one of the 16 129 rank-one maps strictly improves it;
and GF(32) multiplication from 16 to 14. Every count is verified by rebuilding
the algorithm and multiplying it out
([`methods/bilinear_rank/branch_and_bound/what-it-reaches.md`](methods/bilinear_rank/branch_and_bound/what-it-reaches.md)).

**[Lower bounds without a search](core/linear_algebra/tensor_rank_sum.h).** Two
rank-sum bounds return a floor from the tensor alone in milliseconds, and they
are tight often enough to remove the dearest question in a sweep entirely: they
raise GF(16) from 4 to **8** and cyclic convolution from 5 to **9**, each of
which previously cost a minute of exhaustion.

**[Sparsifying the operators](methods/matrix_sparsification/README.md)**, which is the other half
of the cost. Fewer nonzeros means fewer additions, the cost the multiplication
count does not capture. Strassen's encoding operators go from **12 nonzeros to
10**, and on the operators of a published rank-23 ⟨3,3,3⟩ scheme **221 nonzeros
to 128**. That 128 is **the minimum over every change of basis** and not the best
found: the method is `[gottlieb2010]`'s greedy with an exact oracle under it, so
Rado-Edmonds settles it.

**[The rank question as satisfiability](methods/satisfiability/README.md).** Håstad proved
deciding tensor rank NP-complete over every finite field, and that cuts both
ways: `formula_to_tensor` turns 3SAT into a tensor, and three encoders turn the
rank question into one a solver answers. A refutation can be written as DRAT and
checked by `drat-trim`, so a lower bound from a solver is verifiable rather than
trusted.

**[Quotienting by symmetry](methods/bilinear_rank/orbit_reduction/README.md).** A change of coordinates fixing
the target subspace maps solutions to solutions, so one member of each orbit
suffices: **39.2× fewer nodes on a refutation**, and the ⟨3,3,3⟩ candidate pool
collapses from 261 121 to **13 orbits**. It is worth 2.3× on *finding* a
decomposition rather than refuting one, because a proof walks every branch and a
search that stops at the first answer stops before most of the repetition. Both
are re-derived by `evidence/reproduce/measure.py --check`.

**[Isomorph-free enumeration](methods/bilinear_rank/canonical_augmentation/README.md).**
`enumerate-subspaces --canonical` is `[mckay1998]`'s canonical augmentation,
which deduplicates with no memory at all. It returns ⟨2,2,2⟩'s 36 solution
subspaces as the **1 orbit** they are, visiting **22 778× fewer nodes** and
running **11.8× faster**. The group is not walked at all any more: an orbit is
named by least image under a prescribed group, which is what carried the saving
on the clock from under 2× to that, and it is measured rather than glossed
([`deduplication-cost.md`](methods/bilinear_rank/canonical_augmentation/deduplication-cost.md)).

**[Two slices, without a search](methods/pencil_rank/README.md).** A tensor with two slices is a
matrix pencil, and Kronecker's theory gives its minimal indices and elementary
divisors by exact linear algebra in **polynomial time, with no candidate pool**.
What it will not give is a rank: Ja'Ja's formula is a theorem over an
algebraically closed field, and on `(I_4, C)` over GF(2) it says 5 where the
exhaustive search **proves** 6. So the module reports a proved lower bound, a
sharper count marked provisional, and *exact* only where the pencil is
diagonalisable over the field. Twelve pencils settled by exhaustion, three of
which the classical formula gets wrong, are tabulated in
[`methods/pencil_rank/README.md`](methods/pencil_rank/README.md).

**[The rank as a factorisation](methods/canonical_factorisation/README.md).** Take `B`, the
canonical basis of the slice space — the `nm` matrices with a single 1 — and find
the shortest `A` whose rows, read back through `B`, span a space containing the
slices. `B` contributes nothing, and saying so is the useful part: every list of
matrices is `A B` for some `A`, so the whole content is the constraint that
**each row of `A` must read as a rank-one matrix**. With it the least number of
rows is exactly the rank; without it the answer is `dim span(T)` and no search is
needed. The definition is Brockett and Dobkin's `[brockett1978]` and Grigoriev's
`[grigoriev1978]`, and **no novelty is claimed for it**.

What the formulation earns is the answer rather than the search. It calls the
same exhaustive search and does not beat it, but it returns `A` with the recovery
`C`, and **one matrix product checks the pair** without trusting the run that
produced it: GF(4) multiplication comes back as `E11`, `E22` and the all-ones
matrix, which is Karatsuba written as a factorisation. `recovers_slices` is that
check and consults nothing about how the answer was found, and the tests tamper
with one entry of `A` and require the refusal, so the checker is known to be able
to say no. The sweep is a floor, a ceiling, then a walk **upward that never
bisects**, so every question below the answer is a refutation that was completed,
which is what makes the first success minimal rather than merely successful.
**That word is the caveat**: `minimal` goes false the moment `--ceiling` or any
other budget runs out, and the row count is then an upper bound proving nothing
about the rank. `--route sat` never forms the pool at all, so its space is
polynomial in the shape ([`routes.md`](methods/canonical_factorisation/routes.md)).
