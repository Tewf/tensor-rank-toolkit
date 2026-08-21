# Building maps

Two commands write a bilinear map out as a tensor file on standard output, and
between them they are the input side of every other command here: one builds a
map from its definition, the other reads one out of somebody else's algorithm.
Precedence and `BILINEAR_TUNABLES`:
[`precedence-and-tunables.md`](precedence-and-tunables.md).

## `make-tensor`

Its four flags are **modes rather than settings**: exactly one is given, each
takes its own positional arguments, and none has a default to measure. That is
why this is the one command not walked by `cli/arguments.h`, which reads flags
and one positional filename: the words after `--matmul` are the argument. Its
numbers still go through that header's refusals, so `--matmul abc 2 2 2` names
the mode and the word and leaves as **exit 2**; it used to terminate on an
uncaught `std::invalid_argument`. The command asks no question, so it has no "no"
to give: a construction that threw leaves as exit 5 and never as exit 1.

| Flag | Arguments | What it builds |
|---|---|---|
| `--polynomial <p> <left> <right>` | field, two term counts | Multiplying two polynomials over GF(p). `--polynomial 2 5 5` is two 5-term polynomials over GF(2). |
| `--matmul <p> <n> <m> <k>` | field, three dimensions | `⟨n,m,k⟩`: n x m by m x k matrices. `--matmul 2 2 2 2` is where Strassen starts. |
| `--cyclic <p> <length>` | field, length | Multiplying modulo `x^length - 1` over GF(p). |
| `--field <p> <modulus...>` | field, modulus coefficients highest degree first | Multiplying in GF(p^d). `--field 2 1 1 1` is GF(2^2), modulus `x^2 + x + 1`. |

No measurement chose anything here for the modes, and none could: the output is
determined by the arguments, and the maps are the fixtures every timing elsewhere
is taken on rather than something timed themselves.

## `make-tensor`'s options

| Flag | Default | What chose the default |
|---|---|---|
| `--max-memory N` | `2G` | Argument: it leaves room on a 16 GB desktop for a browser and an editor to survive the run. Every mode is cubic in the numbers it takes; `--matmul 2 100 100 100` is 7.2 TiB of slices. |

## `operators-to-tensor`

The map a published ⟨L,R,P⟩ computes, which is the way somebody else's algorithm
gets in. It takes **three positional filenames, L then R then P**, and no flag
but the field, which is the argument shape `PMchecker` and `MMchecker` take so
the two lines differ only in the program name.

| Flag | Arguments | What it does |
|---|---|---|
| `--field <p>`, `-q <p>` | a prime | The field to read the operators over. **No default, and none is possible**: SMS carries no field, the type letter says nothing, and PLinOpt takes it on the command line for the same reason. A missing or composite value leaves as exit 2. |

Nothing else is measurable here either. What it refuses and why: three files that
are not one algorithm leave as exit 5 with `inner dimension mismatch`, his own
words for it, and so does an entry whose denominator vanishes at `p` — which is a
real answer about a rational algorithm and not a defect, since his checkers
reject the same triple at the same primes. The page for whoever has such files is
[`../formats/plinopt_interoperability/exchanging-files.md`](../formats/plinopt_interoperability/exchanging-files.md).
