# Where this repository sits

Written after a proper literature review rather than before one, which is the
wrong order and is why this folder exists. Its job is to keep the repository's
claims inside what is actually unpublished.

It is not the survey. [`writeup/the-research-front/`](../the-research-front/) maps
the field; this takes a position inside that map and says which half of it is
ours to claim. The two were briefly the same filename on two branches, which is
how one path came to hold two documents.

## What the field calls this

The object is the **matrix multiplication tensor**, or more generally a bilinear
map as an order-three tensor; the question is its **rank**, and a decomposition
is a **canonical polyadic decomposition**. Upper bounds for small formats are
catalogued by Sedoglavic at [fmm.univ-lille.fr](https://fmm.univ-lille.fr/).
Searching "tensor rank by SAT" or "bilinear rank" finds the method, not the
field, and finds almost nothing.

| | |
|---|---|
| [`already-published.md`](already-published.md) | the record and the published methods, which are not ours to claim |
| [`where-we-sit.md`](where-we-sit.md) | the one open question the rank strand has a contribution available on, and the first measurement against it |
| [`the-sparsification-strand.md`](the-sparsification-strand.md) | the other strand, where the finding is an absence, and how narrowly to read it |
| [`what-this-changes.md`](what-this-changes.md) | the next flip graph run this position implies |

- [What a GPU would take, part by part](what-a-gpu-would-take.md): the two parts
  the page below never examined, why the `C A` multiply is dead at any speed, and
  why the same measurement says 4% at one shape and almost everything at another.
- [What machine this is the right shape for](hardware-and-parallelism.md): why a
  GPU is the wrong instrument for the tree and backwards for the solver, and
  what the GF(2) bitset representation measured on the hot loop: 6.0x to 39.6x,
  against the 40x to 64x that page had predicted, and it says where the
  prediction went wrong.
