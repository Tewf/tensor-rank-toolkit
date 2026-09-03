# What more threads change, and what they do not

`--threads N` gives the plain exact search one worker per first choice. The
default is one, so nothing this repository publishes is affected by any of the
below, and `MEASURING.md` links here rather than carrying it.

## A refutation is exact. A witness is an upper bound

Ruling `k` out means visiting the whole tree, and it is the same tree whoever
visits it (**whoever, but not in whatever order**): under the orbit quotient the
count depends on the pool's order, measured at 648 against 711 on `⟨2,2,2⟩`
([`../orbit_reduction/what-the-quotient-costs.md`](../orbit_reduction/what-the-quotient-costs.md)).
Threads do not change it, and that is what this section is about. `decide-rank evidence/fixtures/f2_5x5.tensor --target 11` is **459 239 nodes at
1, 2, 4, 6, 8 and 12 threads**, three runs each.

Finding a witness stops the search, and by then the other workers have dispatched
subtrees that a depth-first walk in index order would never have reached. So the
count grows with the workers: on `matmul_2x2x2 --target 7`, **7 436 nodes at one
thread against 42 307 at twelve**.

Those two sentences were published here for a long time as one sentence saying
counts never change. They do, on exactly half the questions.

## The cost is paid out of the shared budget, so it can change the answer

`SearchBudget` is one counter for every worker, so nodes spent in a subtree that
will not win are still nodes spent. With a tight limit that is not a slower yes,
it is a different verdict:

```
decide-rank evidence/fixtures/matmul_2x2x2.tensor --target 7 --node-limit 20000
  --threads 1   ->  7 436 nodes, FOUND 7 products, exit 0
  --threads 4   -> 20 000 nodes, GAVE UP,          exit 3
```

**Use one thread for a satisfiable question, or raise the limit.** There is no
setting of the budget that is right for both, because the sequential search's
whole advantage here is that it never looks at those subtrees.

## What the `found` test is worth

`expand_subspace_impl` tests the shared `found` flag before consuming a node, so a
subtree abandons itself once somebody has a witness. Measured on
`matmul_2x2x2 --target 7`, nodes before and after that test:

| threads | 1 | 2 | 4 | 6 | 8 | 12 |
|---|---|---|---|---|---|---|
| before | 7 436 | 13 706 | 43 823 | 92 143 | 120 326 | 174 715 |
| after | 7 436 | 12 386 | 19 822 | 31 935 | 34 126 | **42 307** |

**4.1x fewer nodes at twelve threads**, and it also removes a slowdown: the same
question ran 4.9x *slower* at twelve threads than at one before the test, 0.0943 s
against 0.0191 s, and now runs in 0.0217 s against 0.0225 s.

It does not restore invariance and cannot: which subtrees are in flight when the
winner reports is a race. That is why the claim above is "upper bound" and not a
number.
