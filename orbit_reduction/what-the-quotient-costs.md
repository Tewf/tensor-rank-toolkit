# What the quotient costs, and where that is worth paying

The orbit quotient removes nodes and charges for the ones it keeps. Both halves
are measurable, and the second is the half that decides when to ask for it.

## Nodes, which are exact

Node counts do not depend on the machine, the load or the clock, so they are
what this page argues from. `⟨2,2,2⟩` over GF(2),
`decide-rank fixtures/matmul_2x2x2.tensor -s matmul 2 2 2`, the default, at
`--target 6` and `--target 7`:

| question | plain | quotiented | nodes removed | seconds |
|---|---|---|---|---|
| refuting k = 6 | 25 399 | **648** | 39.2x | 0.029257 to 0.000992, **about 30x** |
| finding k = 7 | 7 436 | **3 167** | 2.35x | 0.012633 to 0.005903, **about 2.1x** |

The seconds are the ones `descent_search/results.json` publishes, last written by
a full `measure.py` run on 2026-08-23; they were 0.0362 and 0.0214 before the
GF(2) leaf and the reflected Gray walk landed, when the two ratios read 27.8x and
2.14x. **Nothing in the node columns moved and nothing was expected to.**

**The seconds ratios are given as "about" and that is deliberate.** The
quotiented refutation is under a millisecond and the quotiented find is six of
them, so at the small end this is timing the process as much as the search:
[`../MEASURING.md`](../MEASURING.md)'s rule is that a ratio with an end that
small is quoted as an order of magnitude. Two successive regenerations of the
same unchanged code put the first ratio at 29.6 and then 29.5, which is exactly
the digit the rule says not to publish. The node column does not have the
problem, and it is what this page argues from.

The refutation loses far more of its tree than the satisfiable question does,
and that is structural rather than lucky: a refutation visits every node, so
every orbit collapsed is a subtree never entered, while a satisfiable search
stops at the first witness and may well have stopped early anyway.

## The surcharge, which is now small

A quotiented node runs `least_in_orbit` and a plain node does not. That
surcharge is the gap between the two rightmost columns: **about 1.3x** a node at
k = 6, **about 1.1x** at k = 7. Time therefore tracks nodes almost exactly, and the
node table above is very nearly the whole story. It is also why paying fewer
nodes matters more here than paying less per node, which is the finding of
[`what-partial-rejection-leaves.md`](what-partial-rejection-leaves.md).

It has not always been. `least_in_orbit` replaced a `struck` byte array and a
`position` table both sized by the pool, and the leaf on this path was calling
the general Givaro routine rather than the packed GF(2) one. Both are fixed,
and the surcharge that used to make the quotient a real trade is mostly gone.

## The node count stops being a property of the question

**A refutation's node count is order-invariant without the quotient and not with
it.** Measured on `matmul_2x2x2` refuting six products, the same pool handed over
forwards and reversed:

| | in order | reversed |
|---|---|---|
| no group, the control | 25 399 | **25 399** |
| quotiented by the 6 generators | **648** | 711 |

The control is what makes the second row a result. Without a group the tree
counts each *set* of pool elements once, in ascending index order, whatever order
the pool is in. So reversing cannot move it, and does not. With the quotient,
`least_in_orbit` keeps the earliest member of each orbit **within the remaining
suffix**, so reversing changes which element represents each orbit, and different
representatives sit above suffixes of different sizes.

**So the pool's order is a free parameter worth 9.7% here, in the direction
nobody chose.** It is the same lever `--threads` pulls on a satisfiable question,
where it is worth 5.7x, arriving somewhere it was assumed not to reach: the
refutations, which are where a sweep spends its time.

Nothing here says a *better* order exists or how to find one. What it says is
that the search has a knob it does not know it has, and that
[`../exhaustive_search/what-threads-change.md`](../exhaustive_search/what-threads-change.md)'s
"it is the same tree whoever visits it" is true of workers and not of orders.

## What that leaves

**The quotient pays on a refutation and pays less on a satisfiable question**,
which is the same conclusion this page has always reached, now for a different
reason: not because the surcharge is worth it, but because there is barely a
surcharge and the node counts decide alone.

`--symmetry` is still asked for rather than assumed, on the two grounds that
survive: `--symmetry auto` refuses to build 4.06x10⁸ automorphisms for a 4x4
map over GF(2), and the closed-form route needs the caller to say the tensor is
a matrix multiplication, which the file does not record.

Both rows are milliseconds, and [`../MEASURING.md`](../MEASURING.md) counts
those as correctness controls rather than speed evidence. What they are being
used for here is direction and rough size, which they can carry. **Whether the
quotient should become the default is not a question milliseconds can settle**,
and the question is open again now that the surcharge has shrunk.
