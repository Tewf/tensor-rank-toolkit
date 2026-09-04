# evidence/

What is claimed, and how a reader re-derives it. Every published count has
an owner in this group; timings carry their protocol instead.

In this group:

- [`fixtures/`](fixtures/): the maps and operators everything runs
  on; [`plinopt/`](fixtures/plinopt/) is thirteen of PLinOpt's own
  files under CeCILL-B, bytes untouched.
- [`benchmark_tensors/`](benchmark_tensors/): the tensors the
  literature argues about, where each search stops on them, and the one
  owner of what is known about each.
- [`reproduce/`](reproduce/): measure.py, which regenerates every
  published number with its provenance, and the guards CI runs.

How to use, the one command a reviewer runs after building:

```sh
python3 evidence/reproduce/measure.py --build build --check
```

A clean run ends `every published count still reproduces, bar the 5 printed
SKIPPED above`, each SKIPPED line naming its reason.
