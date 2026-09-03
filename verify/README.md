# Checkable claims

A claim the README of this repository makes, written so a solver can check it stays true as
the code moves. One vocabulary (`signature.smt2`), audited once; one claim per file under
`claims/`, each carrying its own `EXPECT`. `sat` means a counterexample exists, so the claim
is false. Run them with `./verify-runner`; CI runs them beside the tests
(`.github/workflows/verify.yml`), and a claim whose verdict stops matching its header fails
the build.

The convention and the escalation (`z3`/`cvc5` for bounded claims, `lean` only when a
bounded `unsat` is genuinely not enough) are the shared verification stack, not this repo's.

## The claims here

- **`no_rref_decomposition_of_witness`** (unsat) and **`witness_tensor_is_rank_two`** (sat)
  are the device-10 soundness counterexample: a 2×2×2 tensor over GF(2) that is rank 2 but
  has no rank-2 decomposition with its first factors in RREF, so fixing the first factors to
  RREF is an unsound constraint on a rank decider. The finding this proves:
  [`../satisfiability/shaped-encodings/device-10-rref-is-unsound.md`](../satisfiability/shaped-encodings/device-10-rref-is-unsound.md).
