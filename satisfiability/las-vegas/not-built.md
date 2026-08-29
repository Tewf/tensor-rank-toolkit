# What was deliberately not built

- **Native XOR inside the continuous solver.** On Brent equations the one
  ingredient the literature shows to matter is the parity kept as a parity
  inside the flip loop, which is xnfSAT's and not ours (`[nawrocki2021]`). A
  Fourier or continuous solver takes an XOR natively as a single monomial with
  no transform, and `multilinear-sat` does not support XOR constraints yet, so
  its turn at these instances comes only with that support. It is the open item
  of this route, and it is stated rather than started here.
- **No `--las-vegas` portfolio mode** running a walk for the yes and kissat for
  the no. Which solver to put in front of kissat is what the table decides, and
  nothing here becomes a default without a number behind it.
- **No higher cutting number for the CNF.** `[nawrocki2021]` found the 6-cut the
  best CNF encoding and the 3-cut `--plain-cnf` writes the worst; the same paper
  found the XNF beats the best CNF on every instance, so the encoding worth
  writing was the XNF, which `--emit-xnf` now is.
- **No flip counts as the machine-independent cost.** Each solver prints its
  own statistic in its own format; wall clock is the one currency they share.
  A count would let `--check` re-derive a Las Vegas row the way it re-derives
  the others, and is the next thing worth adding if the route is kept.
- **No streamlining and no neighbourhood search**, which are the two halves of
  `[heule2019]` that make yalsat productive there. This route is the plain
  formula, which is what challenge 1 asks for.
