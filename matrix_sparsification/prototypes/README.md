# Prototypes: the measurement, before there is an implementation

Four Python scripts that established
[`../method/when-the-matroid-is-regular.md`](../method/when-the-matroid-is-regular.md).
They are kept because a finding whose evidence is deleted is indistinguishable
from a claim, which is the rule the rank strand already follows in
[`../../oracle_guided_search/measurements/`](../../oracle_guided_search/measurements/).

Nothing here is built, tested or wired into a command. They are how the numbers
on that page were obtained, and they are the specification for the C++ that
should replace them.

| script | what it answers |
|---|---|
| `is_it_regular.py` | sample basis determinants; one `|det| ≥ 2` refutes unimodularity |
| `regular_hard.py` | the same, harder, plus Tutte's `Q` / GF(2) / GF(3) rank agreement |
| `spark_by_lp.py` | the minimum weight as `n` linear programmes, via the dual |
| `basis_by_lp.py` | a whole basis from those candidates, and what it weighs |

They need SciPy, which this repository does not depend on and must not start
depending on: run them with `~/miniforge3/envs/orco/bin/python`. An
implementation belongs in [`../../integer_programme/`](../../integer_programme/README.md),
which already keeps a solver chain and needs no new dependency.

```sh
~/miniforge3/envs/orco/bin/python basis_by_lp.py <operator>.sms spark_by_lp.py
```
