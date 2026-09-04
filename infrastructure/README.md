# infrastructure/

How a run happens, bounded: nothing here answers a question about a tensor,
and every command uses all of it.

In this group:

- [`cli/`](cli/): the shared command grammar, the exit-code
  vocabulary (a proved no is 1, a spent budget is 3, neither a crash), and
  the stdout-for-results discipline.
- [`run_limits/`](run_limits/): memory, cores, and which
  processor, plus the card-failure note both card leaves share.
- [`testing/`](testing/): the one assertion helper.
- [`gpu_leaf/`](gpu_leaf/): what one consumer card is worth on the
  leaf test, measured, built only where nvcc exists.
- [`tools/`](tools/): scripts outside the build.

How to use, from a build tree; the output is this run's, quoted as printed:

```
$ build/infrastructure/run_limits/show-limits | head -1
# show-limits: what a run is bounded by here
```
