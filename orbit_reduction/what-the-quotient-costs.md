# What the quotient costs, and where that is worth paying

The orbit quotient removes nodes and charges for the ones it keeps. Both halves
are measurable, and the second is the half that decides when to ask for it.

## Nodes, which are exact

Node counts do not depend on the machine, the load or the clock, so they are
what this page argues from. `⟨2,2,2⟩` over GF(2), `decide-rank`, the default
against `-s matmul 2 2 2`:

| question | plain | quotiented | nodes removed | seconds |
|---|---|---|---|---|
| refuting k = 6 | 25 399 | **648** | 39.2x | 0.0362 to 0.00130, **27.8x** |
| finding k = 7 | 7 436 | **3 167** | 2.35x | 0.0214 to 0.00998, **2.14x** |

The refutation loses far more of its tree than the satisfiable question does,
and that is structural rather than lucky: a refutation visits every node, so
every orbit collapsed is a subtree never entered, while a satisfiable search
stops at the first witness and may well have stopped early anyway.

## The surcharge, which is now small

A quotiented node runs `least_in_orbit` and a plain node does not. That
surcharge is the gap between the two rightmost columns: **1.41x** a node at
k = 6, **1.10x** at k = 7. Time therefore tracks nodes almost exactly, and the
node table above is very nearly the whole story.

It has not always been. `least_in_orbit` replaced a `struck` byte array and a
`position` table both sized by the pool, and the leaf on this path was calling
the general Givaro routine rather than the packed GF(2) one. Both are fixed,
and the surcharge that used to make the quotient a real trade is mostly gone.

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
