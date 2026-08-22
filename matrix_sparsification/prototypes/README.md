# Prototypes: the measurement, before there is an implementation

Four Python scripts that established
[`../method/when-the-matroid-is-regular.md`](../method/when-the-matroid-is-regular.md).
They are kept because a finding whose evidence is deleted is indistinguishable
from a claim, which is the rule the rank strand already follows in
[`../../oracle_guided_search/measurements/`](../../oracle_guided_search/measurements/).

**The C++ they specified now exists**:
[`../lightest_vector_by_simplex.h`](../lightest_vector_by_simplex.h), reached by
`sparsify-operator --simplex`, and it reproduces their numbers exactly. These
stay as the record of how the finding was reached, and as the independent
implementation any future change to that method can be checked against — they use
SciPy and HiGHS where the C++ uses an exact rational simplex, so an agreement
between them is worth something.

| script | what it answers |
|---|---|
| `is_it_regular.py` | sample basis determinants; one `|det| ≥ 2` refutes unimodularity |
| `regular_hard.py` | the same, harder, plus Tutte's `Q` / GF(2) / GF(3) rank agreement |
| `spark_by_lp.py` | the minimum weight as `n` linear programmes, via the dual |
| `basis_by_lp.py` | a whole basis from those candidates, and what it weighs |
| `verify_lp_basis.py` | the same basis back in exact rationals, and the three ranks that say it is the same operator |

They need SciPy, which this repository does not depend on and must not start
depending on: run them with `~/miniforge3/envs/orco/bin/python`. An
implementation belongs in [`../../integer_programme/`](../../integer_programme/README.md),
which already keeps a solver chain and needs no new dependency.

```sh
~/miniforge3/envs/orco/bin/python basis_by_lp.py <operator>.sms spark_by_lp.py
```
