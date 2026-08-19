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

**It is compiled with `PERMLIB_DOMAIN_INT`, and that is not optional here.**
Without it `dom_int` is `unsigned short`, so a permutation domain larger than
65 535 wraps and writes out of bounds. It does not refuse and it does not slow
down: it segfaults inside `Transversal::foundOrbitElement` with a healthy stack,
and a degree only a little past the ceiling could return a wrong answer instead.
This repository's pools reach 261 121 at `⟨3,3,3⟩`, four times the ceiling, so a
build that loses the define is broken for the shapes it exists to reach.

Losing it is easy: a new vendor drop, a second build system, somebody compiling a
file by hand. So it is checked rather than trusted, twice, in
[`../../oracle_guided_search/pool_set_canon.cpp`](../../oracle_guided_search/pool_set_canon.cpp):
a `static_assert` on the type stops such a build at compile time with the reason,
and a run-time check refuses a pool that does not fit the point type rather than
handing PermLib an index that will wrap.

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
