# Two ways to minimise the same sum, and which to trust

Step 3 of the roadmap is solved twice over here:
[`interpolation_programme.h`](interpolation_programme.h) enumerates, and
[`interpolation_by_solver.h`](interpolation_by_solver.h) states the same question
as an integer programme. Both are kept, and this says why.

The model is 25 integer variables at most, one `Equal` row on `deg G` and one
`≤` row per supply of points, because the table caps at degree 10 and
multiplicity 10 and prices only 25 of those hundred cells.

| route | optimum | costs |
|---|---|---|
| `--route built-in` (the default) | proved: exact branch and bound in rationals | flat in `deg G`; 25 variables whatever the degree |
| `--route enumeration` | proved: it walks the whole reachable frontier | quadratic in `deg G`, in time and memory |
| `--route chain` | **feasible, not certified**: an outside solver's point passes the model's own checks, which cannot check optimality | flat, plus 10 to 50 ms to start a process |

**Any feasible selection is already a bound**, because Theorem 2 is an
inequality, so a solver that stops early gives a weaker envelope and never a
wrong one. That is why a suboptimal solve is safe here and would not be in a
rank refutation, and it is why the weakness is discharged by wording rather than
by refusing the answer: the number is reported as `≤`, the backend that produced
it is named, and an uncertified optimum says so on its own line.

**Measured on this machine**, points of degree 1 only, so the enumeration's table
is as large as the degree allows:

| `deg G` | enumeration | chain | built-in |
|---|---|---|---|
| 500 | 0.00 s, 10 MB | 0.02 s | 0.00 s |
| 1000 | 0.02 s, 31 MB | 0.02 s | 0.00 s |
| 2000 | 0.07 s, 115 MB | 0.01 s | 0.00 s |
| 4000 | 0.34 s, 442 MB | 0.01 s | 0.00 s |

So for any `deg G` this method is actually asked for, tens rather than thousands,
**the enumeration wins and the chain loses by one to two orders of magnitude**,
all of it the cost of starting `cbc`. The built-in is the best of the three at
every size. The chain earns its place only past `deg G` of about a thousand,
which is where the enumeration's memory becomes the wall.

**Which is why the built-in is the default, and the chain was.** The table above
settles it twice over: the built-in is fastest at every size *and* its answer is
the only one of the two flat routes that is a proof rather than a point somebody
else vouched for. Defaulting to the chain meant the ordinary run paid 10 to 50 ms
to start a process and got back the weaker claim. The chain is one flag away and
CI still exercises all three, so `external_solver.cpp` keeps a caller.

**And they agree.** `test_interpolation_by_solver` compares the two over 140
questions on ten supplies, 95 of them with an answer, and the enumeration and the
exact branch and bound agree on every one. That test is what makes either route
quotable: the enumeration's exactness had rested on one sentence and five
hand-computed spot checks.

