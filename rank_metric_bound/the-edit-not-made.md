# What it costs, and the one-line edit that is not this module's to make

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

## Why it is still not the default

Because the file is not this module's.
[`../linear_algebra/rank_lower_bound.h`](../linear_algebra/rank_lower_bound.h) is
where the maximum is taken, and a fourth term there is one line every caller gains
at once. A `max` of valid lower bounds is a valid lower bound, so the term can
only raise a floor and never lower one, which is why the six fixtures where
Griesmer loses cost nothing.

What the edit takes, tried rather than guessed:

- `#include "rank_metric_bound.h"`, and a third argument to its `std::max`.
- `target_link_libraries(linear_algebra INTERFACE rank_metric_bound)` in
  [`../linear_algebra/CMakeLists.txt`](../linear_algebra/CMakeLists.txt). The two
  modules then require each other. CMake accepts that between `INTERFACE`
  libraries, in either declaration order, and it was compiled before being written
  down here rather than assumed.
- `linear_algebra/tests/test_rank_sum.cpp` asserts `rank_lower_bound` is exactly
  `max(rank sums, flattening)`. That assertion is what the edit breaks, on
  `f2_5x5` and nowhere else, and the eighteen values it has to become are pinned
  in [`tests/test_rank_metric_bound.cpp`](tests/test_rank_metric_bound.cpp), one
  per fixture, so whoever makes the edit has the numbers rather than a rerun.

**Nothing else moves.** `f2_5x5` is the only fixture whose floor changes, and no
other file in the repository publishes a floor for it: the `12 <= rank <= 14` in
`satisfiability/results.json`, `descent_search/known_ranks.md` and
`state-of-the-art/where-we-stand.md` is the exhaustive search's bracket, which
this reaches rather than improves.
