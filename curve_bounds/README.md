# Bounds from curves

The third strand, and the smallest, because most of it deliberately is not here.

Multiplication in `GF(q^m)` is a bilinear map, so it has a rank, and for large
`m` the best known upper bounds do not come from searching for a decomposition
at all. They come from interpolating on an algebraic curve: pick a curve with
many points of low degree, pick a divisor, and multiplication in the extension
becomes multiplication in a product of much smaller algebras. That is the
Chudnovsky-Chudnovsky method, and `[rambaud2014]` states it as a four-step
roadmap.

Two of those steps are integer arithmetic. Two need algebraic geometry. This
folder is the two that are arithmetic, and it says so rather than gesturing at
the rest.

| Step | What it is | Here? |
|---|---|---|
| 1 | Collect the best bounds on `µ_sym_q(m, l)` for small algebras | [`symmetric_bound_table.h`](symmetric_bound_table.h) |
| 2 | Find curves with many points of low degree | **no** |
| 3 | For a fixed `deg G`, choose the divisor minimising the bound | [`interpolation_programme.h`](interpolation_programme.h) |
| 4 | Check an interpolation system `(G, D, Q)` exists | **no** |

## What Theorem 2 says, and what it costs

`[rambaud2014, Thm. 2]`: if a curve of genus `g` over `F_q` has a closed point
`Q` of degree `m`, and an effective divisor `G = Σ uᵢPᵢ` admits a divisor `D`
with the two conditions below, then

> `µ_sym_q(m) ≤ Σᵢ µ_sym_q(deg Pᵢ, uᵢ)`

The right-hand side depends only on degrees and multiplicities, so minimising it
is an integer programme over a table of published bounds. Step 3 is exactly that,
and it is solved twice over: [`interpolation_programme.h`](interpolation_programme.h)
enumerates the frontier, and
[`interpolation_by_solver.h`](interpolation_by_solver.h) writes the same question
as a model and hands it to whichever MILP solver this machine has. **The default
is `--route built-in`**, the exact branch and bound, because it was fastest at
every size measured and its optimum is a proof where an outside solver's is only
feasibility-verified. Which to trust, which is faster, and the sweep that shows
they agree: [`two_routes.md`](two_routes.md).

The conditions are `l(2D − G) = 0` and `i(D − Q) = 0`. Both are statements about
Riemann-Roch spaces on a specific curve, and neither is checked here.

## Why steps 2 and 4 are not here

They need curve construction and Riemann-Roch space computation. That is Magma
or Sage work; this repository's one dependency is Givaro, which does exact
arithmetic over `GF(p)` and over `Q` and knows nothing about curves. Writing a
half-correct Riemann-Roch would be worse than not writing one, because the
output would look like a bound.

**So a number out of this folder is not a bound on `µ_sym_q(m)`.** It is the
best the method *could* give if a curve with that supply of points exists and
admits an interpolation system. The point supply is an input for exactly that
reason: nothing here can compute it, so nothing here pretends to.

## The table checks out against the other strand

`symmetric_bound_table.h` is `[rambaud2014, Table 1]` transcribed, and three of
its entries can be checked a second way. `µ_sym_2(2)`, `µ_sym_2(3)` and
`µ_sym_2(4)` are 3, 6 and 9; the exact search in [`../exhaustive_search/`](../exhaustive_search/README.md)
reaches exactly 3, 6 and 9 for GF(4), GF(8) and GF(16), from the tensors
themselves and by exhaustion.

That agreement is an observation, not an identity. The table is *symmetric*
rank and the search computes ordinary tensor rank, and symmetric rank is only
known to be at least the rank in general. Where they agree here, they agree.

## Reading the table

An entry is `lower - upper`. A settled entry has them equal. An entry with no
published lower bound reports `lower = 0`, and **zero is not a bound**. It is the
absence of one, which is why `Bound::settled()` exists and why an unpriced point
is refused by the programme rather than costed at nothing.

## Running it

    $ curve-bounds --degree 5 --points 1:8
    supply: 8 of degree 1
    divisor degree: 5, spent exactly
    bound [built-in]: mu_sym_2(m) <= 5, using 5x(degree 1, multiplicity 1)
      an envelope, not a bound: no curve with this supply was shown to exist

Not every supply can pay for every degree. Eight points of degree 1 will not
build a divisor of degree 5 out of points of degree 2 alone, and the command
says so rather than approximating:

    $ curve-bounds --degree 5 --points 2:4
    supply: 4 of degree 2
    divisor degree: 5, spent exactly
    no divisor [built-in]: degree 5 cannot be made from this supply at any price the table publishes

That second run exits 1. A supply that cannot assemble the requested degree at
any price the table publishes is a real answer, not a failure of the command.
