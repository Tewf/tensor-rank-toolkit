# tools/

Scripts that are not part of the build. One lives here today:
`compare_backends.py` asks every backend the same rank question and tabulates
what each one cost, in the only currency they share — wall-clock on one
machine, under [`../MEASURING.md`](../MEASURING.md)'s protocol.

```sh
python3 tools/compare_backends.py --help
```

Its own header says what it measures and what it refuses to conclude.
