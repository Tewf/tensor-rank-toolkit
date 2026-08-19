# PermLib, vendored

Thomas Rehn's PermLib, the `include/` tree only, at upstream commit
`2b4e468e24a059e3659bc095293df0cd363a22f5` (2016-07-16) of
[github.com/tremlin/PermLib](https://github.com/tremlin/PermLib). BSD 3-clause,
[`LICENSE`](LICENSE), authors in [`AUTHORS`](AUTHORS).

**Why it is here.** `permlib_api.h`'s `smallestSetImage` is Linton's *Finding the
smallest image of a set* (ISSAC 2004), credited in
`include/permlib/search/orbit_lex_min_search.h:49`. That is the primitive
[`../../oracle_guided_search/subspace_canon.h`](../../oracle_guided_search/subspace_canon.h)
needs, and writing one instead would be re-deriving a 2004 paper. The review
that reached that conclusion, and the reason nauty is the wrong instrument here
rather than merely a slower one, is summarised in that header.

**Nothing in this tree is modified.** It is upstream, byte for byte, so it can be
diffed against upstream and replaced wholesale. It is header-only: no `.cpp`, so
there is nothing to compile, and it is attached as an INTERFACE target with a
`SYSTEM` include directory so that its warnings are not this repository's.

**Two things it needs from a caller**, both handled in `subspace_canon.cpp` and
neither requiring a patch here:

- `<boost/next_prior.hpp>` must be included **before** any PermLib header.
  PermLib calls `boost::next` in three places and includes nothing that declares
  it; upstream got away with it because older Boost pulled it in transitively.
  A qualified name in a template is looked up where the template is defined, so
  declaring it first is enough.
- **Cycle strings are 1-based.** `Permutation(3, "1 2")` swaps the first two
  points. Passing a 0-based string indexes point `-1` and segfaults rather than
  refusing, which cost an afternoon once and is why this line is here.

Compiles clean at C++20 with GCC 13 under those two rules. `std::binary_function`
in `sorter/base_sorter.h` is deprecated and warned about, not removed, and the
warning is suppressed by the SYSTEM include.
