# Rank of a two-slice tensor, without a search

A tensor with two slices is a **matrix pencil** `A + xB`, and Kronecker's theory
says everything about it that strict equivalence can: a pencil is a direct sum of
singular blocks, indexed by **minimal indices**, and a regular block, described by
its **elementary divisors**. That those four data settle the pencil up to strict
equivalence is `[gantmacher1959, Ch. XII §5, Thm. 5]`, and the canonical form
itself is `[gantmacher1959, Ch. XII §4, (30)]`, restated as `[sumi2009, Lem.
2.1]`; keys are [`../references.md`](../references.md). Both halves are computed
here by exact linear algebra over GF(p), in polynomial time and with no
candidate pool at all.

`decide-rank-by-pencil <tensor>` prints the form and what it implies.

## What is computed, and how strongly it is claimed

| | Claim |
|---|---|
| the Kronecker structure | **exact**, and checked three ways against itself |
| rank over the algebraic closure | **exact**, by `[grigoriev1978, Thm. 1]` and `[jaja1979]`, so a **proved lower bound** over GF(p) |
| rank over GF(p) | **exact** by `[sumi2009, Thm. 3.3]` when the field is large enough for the pencil, or when it is diagonalisable; a proved bound otherwise |
| the projection bound on a bigger tensor | **sound**, and weaker than `rank_lower_bound` |

The structure is checked rather than trusted. The regular part's size is counted
along the rows and along the columns, and separately as the total degree of the
elementary divisors; `kronecker_structure` throws unless all three agree.
## The thing this module was wrong about

It first shipped Ja'Ja's formula read with the **GF(p)** elementary divisors and
called the result the rank. On `(I_4, C)` over GF(2) with `C` the companion of
`x^4 + x + 1` that says 5, and the exhaustive search **proves** there is no
algorithm with five products and exhibits one with six. Twelve pencils settled by
exhaustion, three of which the classical formula gets wrong:
[`the-measured-gap.md`](the-measured-gap.md).

## What closes it, and it is published

The field-size condition this module was written around is the hypothesis of a
theorem and the counterexample is in the same paper, which this repository
already cited. Both are now implemented, and they settle three fixtures outright
that were bounds before and sharpen two others:
[`what-the-literature-settles.md`](what-the-literature-settles.md).

## A second use for the canonical form

`projections_refute` is `[yang2025thesis]`'s `rref` pruner at `k = 2`, and it is
affordable here only because its inner question is a pencil: 105 canonical forms
on `⟨2,2,2⟩` in 0.94 ms, where the source runs a CPD search per plane. It is
sound, it is a millisecond, and **it loses to `rank_lower_bound` on every fixture
where it applies**, for a reason that is precise and fixable:
[`projection-bound.md`](projection-bound.md).

## Files

`polynomial` is arithmetic over GF(p)[x]; `prime_power_factors` turns a
polynomial into the degrees and exponents of its prime powers, deterministically;
`pencil_divisors` diagonalises the pencil over GF(p)[x] twice, forwards for the
finite divisors and reversed for the infinite ones; `minimal_indices` reads the
singular structure off the ranks of one block system; `kronecker_structure` puts
them together and refuses to return anything the three counts disagree about;
`projection_lower_bound` is the one consumer that is not about pencils for their
own sake.
