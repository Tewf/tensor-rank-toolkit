# run_limits/

What one run may take from the machine — memory, cores, and which processor —
so a search stops and says so instead of reaching the allocator or outliving
its terminal. Shared by every method, owned by none.

```sh
show-limits    # what bounds a run here, and where each number came from
```

(An instrument, built in the tree rather than installed; from a build it is
`build/run_limits/show-limits`.) How every strand was audited against these
three budgets: [`adapting-to-the-machine/`](adapting-to-the-machine/README.md).
The one place that asks the operating system what the machine has is
[`machine.h`](machine.h), and the top of [`CMakeLists.txt`](CMakeLists.txt)
maps the rest.
