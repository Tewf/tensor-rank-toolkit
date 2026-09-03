# core/

What everything stands on: the exact arithmetic and the file formats. No
search lives here; both members are linked by every strand.

In this group:

- [`linear_algebra/`](linear_algebra/README.md): matrices, spans in reduced
  row echelon form, and the search-free lower bounds, over GF(p) and over Q
  via Givaro. Nothing here is ever a float.
- [`formats/`](formats/README.md): the tensor file, dense matrices, SMS,
  DIMACS and SMT-LIB, with [`interchange/`](formats/interchange/README.md)
  documenting the exchange with PLinOpt and the FMM catalogue, asserted by
  tests against PLinOpt's own bytes.

How to use: these are libraries; the documentation comment at the top of
each header is the contract, and
[`linear_algebra/README.md`](linear_algebra/README.md) carries a compilable
snippet with the measured cost of the one call both searches spend their
time in (from 29.8 seconds to 11.3 on F3 3×6, measured in
[`linear_algebra/costs.md`](linear_algebra/costs.md)).
