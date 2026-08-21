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

Four panes side by side, all of them on the screen at once: the map, what was
read back, the question, and the answer. Only a pane scrolls, never the page,
because the four are one working state and a reader compares them. `layout.css`
is the shell and `console.css` is what the panes are filled with.

**Start here.** Six worked examples, behind the button of that name, in
[`worked_examples.py`](worked_examples.py). Each fills the map, the tool and the
flags and then stops, so the line is read before it is run. Four of them are the
same map one flag apart, which is the shortest way to see the difference between
found, proved impossible, and a budget that ran out.

1. **The map.** Type it, paste it, load one of the repository's fixtures, open a
   file, or build one with `make-tensor`. The format is the one that already
   exists, `formats/tensor_file.h`, and nothing new was invented. The fixtures
   the chosen tool cannot read are greyed rather than hidden.
2. **The question.** Eleven tools, each stating what it asks. The flags are
   those of `OPTIONS.md`, each carrying the note that says what it costs.
3. **The command.** Shown before you run it and again beside the answer, as a
   line you can retype at a terminal, with a button that copies it. Anything
   worth knowing before you press Run is said underneath it.
4. **The answer.** Three things, in this order: **how it ended**, as the tool's
   own word and as one of found, proved and nothing proved; **the plan the tool
   printed**, its pool, its leaf, its device and its quotient, lifted out of the
   stream; then both streams kept apart as `cli/report.h` splits them, and any
   file the run wrote.

An unmodified fixture is run where it lies, so the line under the answer is the
same one the repository's own documents use. Anything typed is written into the
run's directory first, and that path is what the line then names.

## The first two starters, end to end

```
build/exhaustive_search/decide-rank fixtures/matmul_2x2x2.tensor --target 7
exit 0, FOUND: 7 products, rank bound 6, gap 1;  verified: they compute the map

build/exhaustive_search/decide-rank fixtures/matmul_2x2x2.tensor --target 6
exit 1, NO:    there is no algorithm with 6 products. The search was exhaustive.
```

Two runs and the rank of `⟨2,2,2⟩` is 7, and both lines run unchanged in a
terminal. No timing is quoted, because a wall clock taken beside a browser is
not one `MEASURING.md` would accept.

## What it will not do

It will not turn a budget that ran out into a proof, and
[`what-it-will-not-say.md`](what-it-will-not-say.md) is how that is kept. It
will not leave a search running when you stop it, which is
`run_limits/child_process.h`'s rule applied one level up. What is not finished is
in [`not-production-ready-yet.md`](not-production-ready-yet.md) rather than left
to be discovered.

## Checking it

```sh
python3 web_interface/tests/check_web_interface.py     # 40 checks
```

It starts a console, drives it over HTTP and asserts against real runs of real
binaries: that exit 3 arrives as undecided and never as a refutation, that the
plan lines it promotes are still the characters the tools print, and that each of
the six worked examples still ends where it says it does. It is deliberately not
registered with `ctest`, whose count is quoted elsewhere and means something else.
