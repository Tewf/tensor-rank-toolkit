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
`--question` the fixtures, and `--help` lists the rest.
