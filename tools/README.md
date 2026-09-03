# tools/

Scripts that are not part of the build.

In this folder:

- [`compare_backends.py`](compare_backends.py): asks every backend the same
  rank question and tabulates what each one cost, in the only currency they
  share, wall-clock on one machine, under
  [`../MEASURING.md`](../MEASURING.md)'s protocol. Its own header says what
  it measures and what it refuses to conclude.

Example of use:

```sh
python3 tools/compare_backends.py --build build --timeout 60
```

One row per question and backend; `--only` narrows the backends and
`--question` the fixtures. `--help`'s usage and flag list, quoted as printed
(its description repeats the module docstring above and is elided here):

```
$ python3 tools/compare_backends.py --help
usage: compare_backends.py [-h] [--build BUILD] [--fixtures FIXTURES]
                           [--timeout TIMEOUT] [--repeats REPEATS]
                           [--only ONLY] [--question QUESTION]
[...]
options:
  -h, --help           show this help message and exit
  --build BUILD
  --fixtures FIXTURES
  --timeout TIMEOUT
  --repeats REPEATS
  --only ONLY          substring filter on the backend label
  --question QUESTION  substring filter on the tensor name, so the expensive
                       rows can be run at a different timeout from the cheap
                       ones
```

A full run is timing-sensitive, so it is not reproduced here; the rows this
script produces are already measured, under the same protocol, in
[`../satisfiability/measurements.md`](../satisfiability/measurements.md):
ruling out 6 products on `⟨2,2,2⟩`, the tree search beats the solver by
**11.7x**; ruling out 8 on GF(16), the solver beats the tree by **2.30x**.
