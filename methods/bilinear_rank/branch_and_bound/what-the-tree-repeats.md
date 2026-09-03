# What the tree repeats, and what that settles about McKay

`search_from_above` refuses two moves that open the same child **at one node**:
`residue_of` reduces each candidate against `V`, so two moves with one residue
are one child. Nothing refuses two **nodes** that arrive at one subspace by
adjoining the same moves in a different order. McKay canonical augmentation
(`[mckay1998]`) is the standard answer to exactly that, and this file used to be
answered by pointing at
[`../oracle_guided_search/deduplication-cost.md`](../canonical_augmentation/deduplication-cost.md),
which measures a **different search**. This is the measurement of this one.

Taken 2026-08-23 with `--span-census`, which names each span by
[`subspace_code`](../orbit_reduction/subspace_canon.h): a reduced row echelon
form is unique to a subspace, so counting the duplication needs no canonisation
and no group. The flag changes nothing: every count below is the count of the
run without it.

The flag, run rather than read off the table below:

    lower-the-bound evidence/fixtures/matmul_2x2x2.tensor --span-census

    GF(2), start: 8 products over 4 dimensions
    best: 8 products, rank bound 6, gap 2, verified
    # 21 nodes, 2251 children costed, 3442 moves offered, 0 improvements, 64 branches bounded, depth 3, largest single-move drop 0, tree exhausted
    # span census: 85 nodes entered, 73 distinct spans, 14.1% repeats, most-entered span reached 2 times
    # span census: 21 nodes expanded, 18 distinct spans, 14.3% repeats, most-expanded span reached 2 times
    # span census: 2251 children costed, 1877 distinct spans, 16.6% repeats, most-costed span reached 5 times

which is exactly the `matmul_2x2x2`, width 4 row of the table below.

## One tree, which is all a parent test ever sees

`--rounds 1`, every other flag at its default. **A child is where the money is**:
a node costs `children × p^dim` ranks, so the fourth column decides and the two
before it are context.

| fixture | width | nodes | children | entered | expanded | **children** | most |
|---|---|---|---|---|---|---|---|
| `matmul_2x2x2` | 4 | 21 | 2 251 | 14.1% | 14.3% | **16.6%** | 5 |
| `matmul_2x2x2` | 8 | 40 | 4 359 | 29.6% | 7.5% | **11.1%** | 5 |
| `f2_5x5` | 4 | 5 | 3 194 | 14.3% | 0% | **0.2%** | 2 |
| `f2_5x5` | 8 | 9 | 5 733 | 23.3% | 0% | **0.5%** | 3 |
| `cyclic_f2_7` | 4 | 22 | 17 371 | 14.6% | 9.1% | **9.6%** | 5 |
| `cyclic_f2_7` | 8 | 74 | 57 958 | 46.0% | 27.0% | **28.3%** | 10 |
| `gf32_multiplication` | 4 | 368 | 241 579 | 24.6% | 11.1% | **11.3%** | 7 |
| `gf32_multiplication` | 8 | 1 873 | 1 258 756 | 31.0% | 13.8% | **14.4%** | 9 |
| `gf64_multiplication` | 1 | 7 | 11 653 | 0% | 0% | **0%** | 1 |
| `gf64_multiplication` | 4 * | 10 | 20 339 | 18.5% | 0% | **0.03%** | 2 |
| `gf64_multiplication` | 8 * | 10 | 20 339 | 11.6% | 0% | **0.03%** | 2 |

\* `--nodes 10`. This fixture reaches 19 at `--width 1` and finishes; at 4 and 8
it was stopped at 357 s without returning, which is the row
[`what-width-buys.md`](what-width-buys.md) records as "over 240 s".

Repeats are `(reached − distinct) / reached`, which is what a parent test would
remove: every arrival after the first. `most` is the largest number of times one
span was reached. **So the saving ratio `rho = reached / distinct` is 1.00 to
1.39, and 1.17 on the largest run here.**

## What the rounds add is not McKay's to remove

The default `--rounds 8` restarts from the answer, and on `gf64_multiplication
--width 1` that takes the child repeat from **0% to 40.6%**, 19 607 children
over 11 653 distinct spans. Every one of those is a second round re-walking the
first's ground, and a canonical parent test does not touch it: the test makes
each class reachable from one parent inside **one** augmentation tree, and two
rounds are two trees with two roots. Only a set of everything seen removes it.
On every other row the two agree to within one node.

## Quotienting the moves removes most of it, where a group exists

`--orbit-moves`, one round, against the same fixture unquotiented:

| fixture | width | children | repeat | quotiented | repeat |
|---|---|---|---|---|---|
| `matmul_2x2x2` | 4 | 2 251 | 16.6% | 304 | **1.3%** |
| `matmul_2x2x2` | 8 | 4 359 | 11.1% | 591 | **3.9%** |
| `matmul_2x2x3` | 4 | 159 860 | 12.4% | 34 562 | **18.0%** |

**It removes the duplication by removing the tree, not by deduplicating it**, and
on `matmul_2x2x3` what survives is *more* duplicated than what it replaced. It is
also not free of the beam: `matmul_2x2x2 --width 8` reaches Strassen's 7
unquotiented and stops at 8 quotiented, because the quotient changes which
children the beam ranks. [`orbit_moves.h`](orbit_moves.h)'s losslessness is a
statement about the tree and not about a beam over it.

The group is weaker than the flag suggests, which bounds what this row proves.
`--symmetry matmul 2 2 2` hands over six **generators** of a 216-element group
and `stabiliser_of` filters that list element by element, so the orbits are taken
under the subgroup generated by whichever generators individually fix the node's
span, reported as `stabiliser 0 to 6`. For the other fixtures there is no group
at all: `--symmetry auto` refuses at 9.99872e13 automorphisms for `f2_5x5` and
4.06426e8 for a 4x4 shape, and the closed form is a matrix multiplication one.

## Where the parent test would be cheap, the saving is no larger

`--whole-pool` adjoins any rank-one map of the shape rather than one generated
from the node, so a parent is "this subspace minus one pool element" and
inverting the augmentation is free. That is the shape McKay is built for.

| fixture | width | nodes | children | **children repeat** |
|---|---|---|---|---|
| `matmul_2x2x2` | 4 | 21 | 3 218 | **19.0%** |
| `matmul_2x2x2` | 8 | 25 | 3 907 | **5.8%** |
| `cyclic_f2_7` | 4 | 22 | 334 422 | **18.2%** |

`rho` of 1.06 to 1.23, no better than the generated moves.

## The verdict, priced

[`../oracle_guided_search/canonical_route_price.h`](../canonical_augmentation/canonical_route_price.h)
states the trade: it pays when `rho > pi`, `pi` being one canonical node over one
plain one, and `pi >= 1` by construction. **`rho <= 1.39` here caps the entire
prize at 39% of a run, and it is 10% to 16% on most rows.** Three things then
have to be true at once, and none of them is.

**One, the cheapest possible way of claiming the prize already costs more than
the prize.** Strip the group and the parent test and keep only what any scheme
must do, name each child once, and that is what `--span-census` measures.
Timed under [`../MEASURING.md`](../../../MEASURING.md), fastest of three, load 0.4:

| run | plain | with the census | one child | one name |
|---|---|---|---|---|
| `cyclic_f2_7 --width 4` | 0.64 s | 0.86 s | 36.8 us | **12.6 us** |
| `cyclic_f2_7 --width 8` | 2.09 s | 2.92 s | 36.1 us | **14.2 us** |
| `gf32_multiplication --width 4` | 16.43 s | 18.24 s | 68.0 us | not reported |

Naming a subspace is **34% and 40%** of what costing a child costs, against
savings of 9.6% and 28.3% on those two runs. The `gf32` pair differs by 11%,
inside `MEASURING.md`'s 13% band, so it is not reported as a ratio. The census
recomputes `span_of` from the child's basis and stores whole codes in a
`std::map`, where a scheme meant to pay would build the child's echelon form from
the parent's plus the residue already in hand and hash it; that is several times
cheaper and is **not measured**. What is measured is that the entry fee is a
third of a child and not a rounding error.

**Two, McKay's test is strictly dearer than that name, and here by orders of
magnitude, because this augmentation cannot be inverted.** A move is a
level-lowering summand of an element of `V`
([`level_lowering_moves.h`](level_lowering_moves.h)), so deciding whether a
hyperplane `W` is a **legal** parent of `X` means generating
`level_lowering_moves(W)`, which needs `span_element_ranks(W)`: a `p^(dim−1)`
rank sweep. On `cyclic_f2_7` at the dimension the search reaches that is 4 096
ranks, against the **187** one `minimum_weight_basis_with` costs on this fixture
([`../descent_search/minimum_weight_basis.h`](../greedy_heuristic/minimum_weight_basis.h)).
**One candidate parent costs 22x the child it might cancel**, and a node at that
depth has 15 hyperplanes above the root to choose the canonical one among. This
is the structural difference from the enumerator next door, where a parent is a
set minus an element and inversion is free.

**Three, the memory that makes a parent test preferable to a seen-set is not at
stake.** [`../oracle_guided_search/canonical_augmentation.h`](../canonical_augmentation/canonical_augmentation.h)
reaches for McKay because a seen-set costs one code per object and its objects
number 1.9 million at `<2,2,2>` alone. The largest run above holds 1 077 947
distinct spans of 13 rows by 25 bits: 44 MB stored whole over GF(2), less hashed.
The condition is absent by three orders of magnitude.

**So: no.** The duplication is real and it is 0% to 28% of the children of one
tree. It is worth at most a factor of 1.4, the cheapest instrument that could
collect it measures at a third of a child, and McKay's parent test is 22x a child
on top of that. Where a group does exist, `--orbit-moves` already takes the
quotient at one node in cost.

## What would overturn this

- **A tree two orders of magnitude larger.** The rate grows with the tree
  already: `cyclic_f2_7` is 9.6% at 22 nodes and 28.3% at 74,
  `gf32_multiplication` 11.3% at 368 and 14.4% at 1 873. Both the bit-packing and
  the orbit quotient push toward deeper searches. At `rho` of 3 the seen-set is
  clearly worth building; the parent test still is not, for the inversion reason.
- **Moves from a fixed pool in a search whose trees are large.** `--whole-pool`
  is that augmentation and its duplication measured no larger, but it was
  measured on trees of 21 to 22 nodes. A large one would put the question inside
  the regime `canonical_route_price.h` prices, and that predicate should then be
  asked rather than this file re-read.
- **A fixture with a group this repository will build.** Everything counted here
  is subspaces that are *equal*. Duplication up to `Aut(T)` is a different and
  larger number, bounded by `|G|` rather than by 1.4, and it is the question
  `oracle_guided_search` already answers at 247x to 22 778x.
- **10^8 children in one run**, where a seen-set stops fitting and a memoryless
  test regains its reason to exist. Nothing here is within three orders of that.
