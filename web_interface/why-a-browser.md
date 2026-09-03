# Why a browser, and what the alternatives would have cost

The repository's claim is that **Givaro is the only build dependency and every
solver is optional and found on `PATH` at run time**. A front end is the first
thing that would quietly break that, so the choice was made on dependency cost
before it was made on anything else.

| | New build dependency | New run dependency | Reach |
|---|---|---|---|
| **Local web page, Python 3 standard library** | none | none | any machine that built the toolkit |
| Qt or GTK | Qt 6 or GTK 4, plus their toolchains | the same libraries | desktop only |
| A Python package (Flask, FastAPI) | none | pip, a virtual environment, a wheel per machine | wherever pip is allowed |
| A JavaScript front end with a build step | none | Node, npm, a lockfile, a `node_modules` | wherever npm is allowed |
| WebAssembly, the tools compiled to the browser | Emscripten, a second build of everything | none | anywhere, but it is a second implementation |
| A terminal interface | none | none | no paste of a multi-line map, no file drop |

Python 3 already runs here: `reproduce/measure.py` regenerates every published
number with it, so a machine that can reproduce a result can serve this. The
standard library has `http.server`, `subprocess` and `json`, which is all of it.
A browser is the one graphical toolkit that is already installed everywhere,
including on a machine reached over `ssh -L`, where a Qt window is not.

The last row is the one worth stating plainly. Compiling the tools to
WebAssembly would remove the server entirely and would also produce a second
build of the mathematics, whose answers would have to be trusted alongside the
first. This interface shells out to the binaries the test suite checks, so there
is one implementation of every answer and it is the one that was verified.

That claim is checked rather than assumed. Read straight from `build/`, the same
binary the checks use:

```
build/descent_search/minimise-rank fixtures/f2_5x5.tensor
exit 0, algorithm: 14 products, rank bound 12, gap 2
```

14 is the number the fifth worked example in
[`worked_examples.py`](worked_examples.py) shows behind a button, and the number
[`tests/check_web_interface.py`](tests/check_web_interface.py) asserts of the
same step inside the pipeline flow. One binary, one answer, wherever it is
reached from.

## What it does not solve

A browser is a poor place for a search that runs for hours, because a page can
be closed while a run is going. The run survives that, and a reloaded page finds
it again, because the run belongs to the server rather than to the tab. What it
does not survive is stopping the server, which is deliberate: nothing of a run
may outlive the thing that started it. The rest of what is unfinished is in
[`not-production-ready-yet.md`](not-production-ready-yet.md).
