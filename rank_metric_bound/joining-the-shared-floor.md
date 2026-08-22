# What it costs, and what moved when it joined the shared floor

Both bounds want the `|F|^n_d` contraction ranks of an axis: the table
`rank_sum_lower_bound` already builds. Given that table each is one linear scan,
which is why the per-axis entry points take the table and not the tensor, and why
this is **strictly cheaper than the floor already in use**.

Both timed in one process, one core, fastest of three, under
[`../MEASURING.md`](../MEASURING.md): on `f3_3x6`, Griesmer **4.1 ms** against
`rank_lower_bound`'s **498 ms**; on `f2_3x8`, **0.9 ms** against **29.8 ms**. The
difference is the line bound enumerating pairs where this reads the table once,
and it confirms the 469 ms `../linear_algebra/tensor_rank_sum.h` records for
`f3_3x6` with process start included. On `gf16` the two are 0.017 and 0.027 ms,
inside the 13% band, so that pair is not quoted as a ratio.

## It is the default now, and this is what that took

`linear_algebra/rank_lower_bound.h` is where the maximum is taken, so a fourth
term there is one line every caller gains at once. A `max` of valid lower bounds
is a valid lower bound, so the term can only raise a floor and never lower one,
which is why the six fixtures where Griesmer loses cost nothing.

Three edits, all made:

- `#include "rank_metric_bound.h"` in that header, and a fourth argument to its
  `std::max`. Note the namespace is `rank_metric_bound`, not `bilinear_rank`,
  which is a wrinkle worth knowing before writing the call.
- `target_link_libraries(linear_algebra INTERFACE ... rank_metric_bound)`. The two
  modules then require each other, and CMake accepts that between `INTERFACE`
  libraries in either declaration order. Compiled before being written down.
- Two test expectations, on `f2_5x5` and nowhere else.
  `descent_search/tests/test_pipeline.cpp`'s floor column goes 10 to 12.
  `linear_algebra/tests/test_rank_sum.cpp` asserted `rank_lower_bound` is
  **exactly** `max(rank sums, flattening)`, and that is what the edit breaks; it
  now asserts the shared entry point is **at least** that, because pinning it to
  an exact value there would either duplicate this module's own test or tie that
  file to a bound it does not compute.

**Nothing else moved, and this was checked rather than argued.**
`reproduce/measure.py --check` reports that every published count still
reproduces. `f2_5x5` is the only fixture whose floor changes, and no other file
publishes a floor for it: the `12 <= rank <= 14` in `satisfiability/results.json`,
`descent_search/known_ranks.md` and `the-research-front/where-we-stand.md` is the
exhaustive search's bracket, which this reaches rather than improves. That
bracket was `13 <= rank <= 14` once `--target 12` was exhausted and is `13` since
[`lower-the-bound`](../incumbent_search/README.md) exhibited one; the sentence
above is what was true when the floor moved.
