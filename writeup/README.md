# writeup/

The argument, as opposed to the machinery. Nothing in this group is linked
by the build; everything in it cites the code and the records that are.

In this group:

- [`article/`](article/README.md): the formal statement: definitions,
  theorems, proofs, the space argument, and the negative results.
- [`how-the-search-works/`](how-the-search-works/README.md): the exact
  search in pseudocode, every parameter, and the verdict on wiring each
  piece.
- [`positioning/`](positioning/README.md): what this library adds to the
  research front, and what it does not.
- [`the-research-front/`](the-research-front/README.md): where the field
  stands, about the field and not about us.

How to use: read [`how-the-search-works/`](how-the-search-works/README.md)
beside a run of `decide-rank --trace`, whose tree it explains line by line;
the article builds with `latexmk -pdf` in [`article/`](article/README.md).
