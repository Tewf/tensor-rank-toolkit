# What the beam width buys, on every fixture, and where it changes nothing

`--width N` enters the `N` cheapest children of each node and discards the rest,
so it is the dial between a greedy walk and the branch and bound `--width 0`
runs. **A beam is incomplete**: what it discards may have held the answer, which
is why this search only ever exhibits a decomposition and never refutes one.

Swept 2026-08-22 at widths 1, 2, 4, 8 and 16, `--from descent` and every other
flag at its default. **Counts, not seconds**, except where a run did not finish
at all, which is a fact about this machine and is marked as one.

| fixture | 1 | 2 | 4 | 8 | 16 | width that first reaches the best |
|---|---|---|---|---|---|---|
| `matmul_2x2x2` | 8 | 8 | 8 | **7** | 7 | **8** |
| `f2_5x5` | 14 | 14 | 14 | 14 | 14 | 1 |
| `f2_3x8` | 15 | 15 | 15 | 15 | 15 | 1 |
| `f2_4x7` | 16 | 16 | 16 | 16 | 16 | 1 |
| `f3_3x6` | 10 | 10 | 10 | 10 | 10 | 1 |
| `cyclic_f2_5` | 10 | 10 | 10 | 10 | 10 | 1 |
| `cyclic_f2_7` | 13 | 13 | 13 | 13 | 13 | 1 |
| `gf16_multiplication` | 9 | 9 | 9 | 9 | 9 | 1 |
| `gf32_multiplication` | 15 | 14 | 14 | **13** | over 240 s | **8** |
| `gf64_multiplication` | **19** | over 240 s | over 240 s | over 240 s | over 240 s | **1** |

**Width changes the answer on two fixtures of ten, and on both of them it is 8
that changes it.** Nothing here wants 16: every fixture that finishes at 16
returns what it returned at 8. That is the measurement behind `--width auto`
doubling once and stopping rather than climbing.

**It is not free, and the cost is where the gain is.** `gf32_multiplication` is
10, 44 and 368 nodes at widths 1, 2 and 4, all of them seconds; at width 8 it is
**1 873 nodes and 466 s on one core**, which is the same 1 873 nodes
[`what-it-reaches.md`](what-it-reaches.md) publishes. So a default of 8 would turn
a run that answers in seconds into one that answers in eight minutes, on every
fixture, to change the answer on two. That is why the default stays 4 and the
widening is a flag.

## `--width auto`, and the three things it waits for

It runs at 4, and doubles **once** only when all three hold:

- **the tree was exhausted**, not the budget. A run that ran out of nodes can be
  helped by more nodes, and widening is the wrong lever for it;
- **the budget is mostly unspent**, so there is room to pay for the wider tree;
- **the answer is still above `rank_lower_bound`**, so there is something left to
  reach.

`gf32_multiplication` at width 4 exhausts its tree at 368 nodes of 20 000 with 14
against a floor of 12: all three, and widening reaches 13.
`matmul_2x2x2` exhausts at 21 nodes with 8 against a floor of 6, and widening
reaches Strassen's 7. On the eight fixtures where width buys nothing the
condition still fires, and costs a second run of a tree that is one or two nodes.

## The row that changed while this was being measured

**`gf64_multiplication` is 19, not the 20 published until 2026-08-22**, and the
narrowest beam is what reaches it: 19 products in 10 nodes at `--width 1`, 106 s,
tree exhausted. Verified twice, by the tool rebuilding the map before printing
and then by `operators-to-tensor` reading the emitted ⟨L, R, P⟩ back and
rebuilding the fixture exactly.

**A narrower beam reaching further is not a paradox, it is what a beam is.** The
children are entered cheapest-first, and cheapest-first is a heuristic: at width 1
the search commits to that ordering and walks deep, at width 4 it spends the same
budget spreading across children that the ordering ranked well and the answer did
not need. Nothing here says width 1 is better in general — it is worse on two of
the ten — only that neither direction is monotone, which is the honest thing to
know about a dial before turning it.

`mu_2(6) = 15` is still 4 below this. The gap is real and this does not close it.
