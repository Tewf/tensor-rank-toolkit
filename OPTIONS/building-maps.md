# Building maps

`make-tensor` writes a bilinear map out as a tensor file, on standard output.
It is the input side of every other command here. Precedence and
`BILINEAR_TUNABLES`: [`precedence-and-tunables.md`](precedence-and-tunables.md).

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

No measurement chose anything here, and none could: the output is determined by
the arguments, and the maps are the fixtures every timing elsewhere is taken on
rather than something timed themselves.
