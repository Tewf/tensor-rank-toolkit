# Bounding from a curve

`curve-bounds` answers a different question from every search here, and
`list-solvers` reports the backend chain it may hand that question to.
Precedence and `BILINEAR_TUNABLES`: [`../OPTIONS.md`](../OPTIONS.md).

## `curve-bounds`

| Flag | Default | What chose the default |
|---|---|---|
| `--degree G` | none; required | Nothing to measure. The divisor's degree is spent exactly and not as a budget: a `<=` would always answer "one rational point, cost 1", which is a bound on nothing. |
| `--points d:n` | none; required | Nothing to measure. This is step 2's output, and step 2 is not in this repository. |
| `--table` | off | Nothing to measure: prints `[rambaud2014, Table 1]` as transcribed and stops. |
| `--route built-in\|chain\|enumeration` | `built-in` | Measured, and read the measurement honestly. On degree-1 supplies at G = 500, 1000, 2000 and 4000 the built-in takes **0.00 s at every size**, the chain 0.01 to 0.02 s, and the enumeration 0.00 to 0.34 s and 10 to 442 MB. The built-in column being all zeros shows it is **not slower**; it does not rank the two. What settles the default is the second reason, which is not a timing: the built-in's optimum is a proof, and an outside solver's is only feasibility-verified. The chain's loss is 10 to 50 ms of process startup. `--route chain` **was** the default, and the measurement is why it is not. |
| `--node-limit N` | `ilp_node_limit`, `200000` | **Nothing.** No measurement anywhere. Reaching it returns `Exhausted`, which falls back to the dynamic programme rather than bounding anything. |
| `--solver-timeout N` | `ilp_time_limit_seconds`, `300` | **Nothing measured.** It exists because of an incident rather than a table: five leaked solvers once sat at full tilt for half an hour and spoiled another session's measurements. It bounds `--route chain` only; the built-in has no wall clock anywhere. |
| backend order (no flag) | `ilp_backend_order` | **Nothing measured.** Gurobi leads where a licence exists and the built-in trails because it is the slowest. `--route built-in` is how a caller sidesteps the order entirely. |

Agreement, which is measured and is a correctness result rather than a timing:
the three routes were compared on **140 questions over ten supplies, 95 of them
with an answer**, and all agreed.

## `list-solvers`

No flags but `--help`. It prints the integer programme backends in preference
order and whether each is on `PATH`. Anything else on the line is refused as
exit 2: `main` took no arguments at all, so every word was dropped in silence
and `list-solvers --help` printed the table and left as 0.

It reads `ilp_backend_order` from `tunables.conf`, because this command's whole
output **is** the ranking: printing the compiled order while the file had moved
it would make the one place a caller looks the one place that lies.

The built-in is always present and is the only backend whose answer needs no
checking, so it is also the only one that may report a problem infeasible. That
is an argument about what can be believed, not a measurement, and it is why the
built-in's position in the chain is not the file's to choose: `solve` reaches it
last whatever the order says.
