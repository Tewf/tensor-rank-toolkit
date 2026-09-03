# Plan: orbit methods, and where they belong in this repository

**This plan has been carried out, under different names.** It was written on a
branch that never held the code, so read it for the argument and the sources, not
as work outstanding. Its four items landed here as:

| planned | built as |
|---|---|
| 1. `symmetry_group` and `stabilises` | [`automorphism.h`](../automorphism.h) |
| 2. orbits of the pool by union-find | [`pool_orbits.h`](../pool_orbits.h) |
| 3. generators, each verified before use | [`group_construction.h`](../group_construction.h) |
| 4. wired into `expand_subspace`, same answers | [`orbit_search.h`](../orbit_search.h), proved by [`tests/test_symmetry_agreement.cpp`](../tests/test_symmetry_agreement.cpp) |

What it says about the heuristic being a separate question, and about the pool
having to be closed under the action, still governs.

What it built to, concretely: `decide-rank evidence/fixtures/matmul_2x2x2.tensor
--target 6 -s matmul 2 2 2` visits 648 nodes where the unquotiented walk visits
25 399, [`../what-the-quotient-costs.md`](../what-the-quotient-costs.md)'s
table.

The published continuation of the algorithm in
[`exhaustive_search.h`](../../exhaustive/exhaustive_search.h). Source:
`[covanov2018]` §1.3, *RP-automorphisms*, and §2.2.4, *Improving on BDEZ using
RP-automorphisms*, whose `[covanov2018, Alg. 6]` is `BDEZStab` and is
`[covanov2019, Alg. 3]` reprinted. §2.2.4 opens *"we present in this section,
with kind permission from the authors, an unpublished improvement to Algorithm
BDEZ"*, the authors being `[bdez2012]`'s, whose own conclusion names "using the
symmetries of the problem" as the thing it did not do. **The thesis has been
read**; where it and the paper agree, the paper is cited, and where they differ
[`the-group.md`](the-group.md) says so.

| | |
|---|---|
| [`the-group.md`](the-group.md) | the action, the setwise stabiliser of the span, the two results that make a quotient lossless, and why the thesis's `RPA` is not the group implemented |
| [`the-algorithm.md`](the-algorithm.md) | `BDEZStab`, which prunes at every node, and the order it was built in |
| [`generators.md`](generators.md) | where `Stab(T)` comes from, and why every element is verified before use |
| [`what-it-is-worth.md`](what-it-is-worth.md) | what the quotient buys, where to aim it, and why the heuristic is a separate question |
