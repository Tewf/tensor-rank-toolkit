# What a node cannot tell you

The exact search tests one thing at a node: `dim V` against the target. That
looks like an omission, and four separate attempts to add a second test have now
been measured and closed. They are collected here because each is the obvious
idea, and the obvious idea keeps being had.

All four are **sound**. None of them **fires**.

## 1. A lower bound on the span

> at every node compute `rank_lower_bound(V)`, and prune when it meets the target

Measured over seven levels on three fixtures:
`rank_lower_bound(V) == max(dim V, its value at the root)`, exactly. Pinned in
[`tests/test_bound_at_a_node.cpp`](tests/test_bound_at_a_node.cpp).

Every term degrades for one reason. The flattening along the axis the search
grows **is** the dimension, and Griesmer reads the least rank in the space, which
the search drives to 1 with its first choice. The root value is the floor a
search already starts from, so the test is true exactly when `dim V >= target`
is, which costs nothing.

**The bound asks what the cheapest thing in the space is, and the search spends
its time putting cheap things there.**

## 2. The total rank over the span

> average the ranks rather than taking the least

That is Laskowski's bound, `[yang2025thesis, Thm. 3]`, already inside
`rank_lower_bound`. Its numerator is at most `(q^k - 1) * min(n,m)` and its
divisor is `q^(k-1)(q-1)`, so it can never exceed `q/(q-1) * min(n,m)`: **a
constant in the depth**. On `f2_5x5` that ceiling is 10, it is saturated at the
root, and `dim V` passes it at depth 1.

The bound that reads the most of `V` has the lowest ceiling, and no better code
moves it: it follows from `rank <= min(n,m)`.

## 3. An incumbent instead of a sweep

> start from an upper bound and tighten, rather than sweeping up from the floor

Asking `--target t` prunes at `dim V > t`, which **is** `dim V >= best` with
`best = t + 1`. So the sweep is already a branch and bound, and its incumbent is
set by the refutations it has completed rather than by anything exhibited. No
construction can match that: `t + 1` has only to be **not yet refuted**, where an
upper bound has to be **built**.

Seeding from `minimum_weight_basis` starts at 8 against 7 on `<2,2,2>` and **16
against 13** on `f2_5x5`, three extra levels of an exponent paid up front. And
the loop's re-work is not what people expect: refuting 11 on `f2_5x5` is 459 239
nodes against 146 402 553 for refuting 12, so the repetition is **0.31%**.

## 4. Meeting the floor with `cost(V)`

> `cost(V)` rank-one maps span something containing `span(T)`, so it is a real
> solution: return it when it reaches the target, and stop outright when it
> reaches the floor

Sound, and it cannot cause a false `NO`, since it only ever finds solutions.
Measured, the cheapest `cost(V)` seen at each depth:

| question | floor | target | by depth |
|---|---|---|---|
| `matmul_2x2x2` | 6 | 7 | **8, 8, 8** over 4000 nodes |
| `gf8_multiplication` | 6 | 6 | **9, 7, 7** over 1226 nodes |
| `f2_2x3` | 5 | 5 | **5 at the root** |

It fires only where the minimum-weight basis is already the answer, and there the
search is two nodes anyway. **`cost(V)` does fall as the search descends** — the
`gf8` row above goes 9 to 7 — which this file denied until 2026-08-20; the
minimal case is in
[`../descent_search/sorted_span.h`](../descent_search/sorted_span.h). What it
never does here is fall far enough to reach the target before `dim V` does. And
it cannot help a **sweep** whatever it does, because every question below the
rank is a refutation, where `cost(V) <= k` would exhibit the very algorithm that
question denies: it can fire only on the one satisfiable question, while costing
`p^dim` ranks a node on all the refutations under it.

## What the four have in common

Every one reads `V`, and `V` is the part already paid for. A criterion that does
not degrade has to read the **residual**: given the maps committed, what stands in
the way of finishing. The one quantity here that is monotone under the search's
own moves is the deficit,
[`generating-candidates-from-the-span.md`](generating-candidates-from-the-span.md),
and turning it into a bound needs a limit on how much one adjunction can buy,
which is a count of rank-one matrices in a coset, which is a Segre intersection.
That is open.
