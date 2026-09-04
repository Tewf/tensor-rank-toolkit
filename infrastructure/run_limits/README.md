# infrastructure/run_limits/

What one run may take from the machine: memory, cores, and which processor.
Shared by every method and owned by none, so a search stops and says so
instead of reaching the allocator or outliving its terminal.

In this folder:

- [`machine.h`](machine.h): the one place that asks the operating system what
  the machine has, so defaults derive from the machine instead of being
  fitted to one laptop.
- [`memory_budget.h`](memory_budget.h): the one place that decides how much a
  run may ask for, and refuses the rest before the allocator sees it.
- [`parallel.h`](parallel.h): the worker pool behind `--threads`.
- [`device.h`](device.h): the cpu/gpu ranking behind `--device`.
- [`child_process.h`](child_process.h): solvers run in a process group of
  their own, with the alarm that ends them; its header explains the one
  window that leaves open.
- `show_limits.cpp`: the `show-limits` instrument.
- [`adapting-to-the-machine/`](adapting-to-the-machine/): the audit
  of every strand against all three budgets.
- [`tests/`](tests): the budgets asserted against real runs.

Example of use, from a build tree (an instrument, deliberately not
installed):

```sh
build/infrastructure/run_limits/show-limits
# show-limits: what a run is bounded by here
#
# machine, as the kernel reports it
#   cores                   12                    hardware_concurrency()
#   physical memory         31.0 GiB              MemTotal
```
