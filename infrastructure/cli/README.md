# infrastructure/cli/

The infrastructure every command shares. Nothing in here answers a question
about a tensor; each header holds one convention, used by all thirteen
tools.

In this folder:

- [`arguments.h`](arguments.h): the command-line walk, written once, so every
  refusal names the flag and quotes the word.
- [`exit_code.h`](exit_code.h): the exit vocabulary. Yes is 0, a proved no is
  1, a line that did not parse is 2, a spent budget is 3, a run that could
  not start is 5. Neither 1 nor 3 is a crash.
- [`report.h`](report.h): commentary goes to stderr with `#` in front,
  results go to stdout, bare.
- [`size_argument.h`](size_argument.h) and
  [`symmetry_argument.h`](symmetry_argument.h): the two typed values, `2G`
  and `matmul 2 2 2`.
- [`timing.h`](timing.h): the one clock every printed duration comes from.
- [`tunables.h`](tunables.h): reads [`../../tunables.conf`](../../tunables.conf),
  the numbers a run is bounded by.
- [`interrupt_cleanup.h`](interrupt_cleanup.h): removes a scratch file when a
  signal cuts a run short.
- [`tests/`](tests): asserts all of the above against the built binaries,
  not against the headers alone.

Example of use, the loop every command is built on (the worked example at the
top of [`arguments.h`](arguments.h) is the full version):

```cpp
cli::Arguments arguments(argc, argv);
while (arguments.next_flag()) {
    if (arguments.is("--target")) target = arguments.whole_number();
    else arguments.refuse();   // names the flag, leaves as exit 2
}
```
