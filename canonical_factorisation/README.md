# The rank as a factorisation over the canonical basis

Given `T` with slices in `F^{n x m}`, take `B`, the canonical basis of that
space: the `nm` matrices with a single 1. Find `A` such that the rows of `A B`
span a space containing the slices, with `A` as short as possible.

```sh
factor-over-canonical-basis fixtures/gf4_multiplication.tensor
```

```
floor: 3 (proved)
A, over the canonical basis: 3x4      C, the recovery: 2x3
  1 0 0 0                               1 0 1
  0 0 0 1                               1 1 0
  1 1 1 1
checked: every row of A has rank 1, and C A is the tensor
components: 3 (the rank)
```

Read `A` back through `B`: its rows are `E11`, `E22` and the all-ones matrix.
Each is rank one, and `C` says the first slice is rows 1 and 3 added, the second
is rows 1 and 2. That is Karatsuba for GF(4), as a factorisation.

## Where the difficulty is, and where it is not

**`B` contributes nothing, and saying so is the useful part.** `A B` is the
reading map, taking a row of coefficients to the matrix whose entries they are,
so every list of matrices is `A B` for some `A`. Choosing the canonical basis
rather than another only fixes which coordinates the entries are written in.

The whole content is the constraint that **each row of `A` must read as a
rank-one matrix**. With it, the least number of rows is exactly the rank of `T`;
without it, the answer is `dim span(T)` and no search is needed. Everything
below is about the constraint.

## Whose definition this is

This is bilinear rank as Brockett and Dobkin `[brockett1978]` and Grigoriev
`[grigoriev1978]` defined it: the least number of rank-one matrices whose span
contains the slice space. Ja'Ja' `[jaja1979]` and Byrne and Cotardo
`[byrne2021]` restate it. **No novelty is claimed for the formulation.** The
equivalence with the sum-of-rank-one-tensors definition is two lines each way:
a decomposition's terms span a space containing the slices, and a spanning set
of rank-one matrices expresses each slice as a combination of them, which is `C`.

What the formulation earns is not a better search but a better **answer**: `A`
and `C` together are checkable by one matrix product, by a reader who did not
run the search and has no reason to trust it. `recovers_slices` is that check,
and it consults nothing about how the answer was found. The tests tamper with
one entry of `A` and require the refusal, so the checker is known to be capable
of saying no.

## How the rows are found

By the strongest route this repository has for the shape rather than by a search
of its own, and the choices are recorded in [`routes.md`](routes.md). The one
worth knowing here: the sweep runs **upward** from the proved floor and never
bisects, so every question below the answer is a refutation that was completed,
which is what makes the first success minimal rather than merely successful.
`minimal` goes false the moment any budget runs out, and the count is reported
as an upper bound instead.

## Two routes

`--route exhaustive` materialises the pool of rank-one maps; `--route sat` never
forms one, so its space is polynomial in the shape. `--route auto` takes the
solver past 20 000 pool matrices. Which to use, what each costs, and why
`GL(n) x GL(m)` cannot shrink the pool: [`routes.md`](routes.md).

## What it does not do

It does not beat `decide-rank`, and is not meant to: it calls the same search.
For two slices, `pencil_rank/` answers a related question in polynomial time
without any pool at all, and where the two overlap they agree, which the tests
check on `pencil_split_f3_3` and `pencil_nilpotent_f2_3`.
