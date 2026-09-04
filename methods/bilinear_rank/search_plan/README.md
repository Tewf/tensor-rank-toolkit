# methods/bilinear_rank/search_plan/

The seven choices a run makes about how it will be carried out, collected in
one place that owns no rule of its own. `--plan-out` writes them down and
`--plan-in` replays them, so the same run on other hardware differs in its
machine and not in its decisions.

In this folder:

- [`search_plan.h`](search_plan.h): the seven choices (pool, leaf route,
  device, and the rest) and the three rules that settle them, each rule
  asked of the module that already owned it.
- [`plan_file.h`](plan_file.h): one plan written down and read back,
  `name value` per line.
- [`tests/`](tests/): a plan replayed is the same run, asserted.

Example of use:

```sh
decide-rank evidence/fixtures/f2_2x2.tensor --plan-out plan.txt
head -6 plan.txt
# A search plan, from decide-rank --plan-out. --plan-in replays it.
# `name value`, one per line; # starts a comment. Every field is written,
# so a plan says what it chose and never what it left to a rule.
pool materialised
leaf_route auto
device cpu

decide-rank evidence/fixtures/f2_2x2.tensor --plan-in plan.txt   # the same choices, replayed
```
