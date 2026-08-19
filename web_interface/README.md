# A console for the twelve tools

A local web interface that takes a bilinear map typed, pasted or loaded from a
file, runs any of the twelve command-line tools on it, and shows the answer with
the exact command that produced it. It shells out to the built binaries and
implements no mathematics of its own.

## Running it

Two steps, and the first is the repository's own.

```sh
cmake -B build -G Ninja && cmake --build build   # Givaro is the only dependency
python3 web_interface/serve.py                   # opens http://localhost:8770/
```

Nothing else is installed: the server is the Python 3 standard library and the
interface is a browser, so it needs exactly what building the toolkit needed.
[`why-a-browser.md`](why-a-browser.md) prices the alternatives.

It listens on the loopback address, so only this machine can reach it, and
`ssh -L 8770:localhost:8770 host` is how it drives a run on a server. The flags
are `--build`, `--port`, `--host`, `--runs` and `--wall-clock`; `--help` lists them.

## What you do with it

1. **The map.** Type it, paste it, load one of the repository's fixtures, open a
   file, or build one with `make-tensor`. The format is the one that already
   exists, `formats/tensor_file.h`, and nothing new was invented.
2. **The question.** Eleven tools, each stating what it asks. The flags are
   those of `OPTIONS.md`, each carrying the note that says what it costs.
3. **The command.** Shown before you run it and again beside the answer, as a
   line you can retype at a terminal.
4. **The answer.** The exit code in the vocabulary of `cli/exit_code.h`, the
   tool's own reading of it, both streams kept apart as `cli/report.h` splits
   them, and any file the run wrote.

An unmodified fixture is run where it lies, so the line under the answer is the
same one the repository's own documents use. Anything typed is written into the
run's directory first, and that path is what the line then names.

## One example, end to end

Load `matmul_2x2x2.tensor`, choose `decide-rank`, ask for 7 products, run:

```
build/exhaustive_search/decide-rank fixtures/matmul_2x2x2.tensor --target 7
exit 0, YES:  7 products, rank bound 6, gap 1;  verified: they compute the map
```

Change the target to 6 and run it again:

```
build/exhaustive_search/decide-rank fixtures/matmul_2x2x2.tensor --target 6
exit 1, NO:   there is no algorithm with 6 products. The search was exhaustive.
```

Two runs and the rank of `⟨2,2,2⟩` is 7: one algorithm at 7 that was checked,
and no algorithm at 6 by a search that was exhaustive. Both lines run unchanged
in a terminal and give the same two answers. No timing is quoted here, because a
wall clock taken beside a browser is not one `MEASURING.md` would accept.

## What it will not do

It will not turn a budget that ran out into a proof, and
[`what-it-will-not-say.md`](what-it-will-not-say.md) is how that is kept. It
will not leave a search running when you stop it, which is
`run_limits/child_process.h`'s rule applied one level up. What is not finished
is written down in
[`not-production-ready-yet.md`](not-production-ready-yet.md) rather than left to
be discovered.

## Checking it

```sh
python3 web_interface/tests/check_web_interface.py     # 21 checks, about 15 s
```

It starts a console, drives it over HTTP and asserts against real runs of real
binaries, including that exit 3 arrives as undecided. It is deliberately not
registered with `ctest`, whose count is quoted elsewhere and means something else.
