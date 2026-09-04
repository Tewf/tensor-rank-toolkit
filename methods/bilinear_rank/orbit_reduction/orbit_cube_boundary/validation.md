# Validation, which is the deliverable

No refutation built on cubes is believed until both of these pass.

| test | what it establishes |
|---|---|
| `ctest -R "^orbit_cubes$"` | representatives partition the pool, cubes are well formed, a wrong shape is refused |
| `ctest -R orbit_cubes_preserve_the_answer` | whole and split agree, at 7 products where `⟨2,2,2⟩` is satisfiable and 6 where it is not, **with the term ordering off and on**, plus `⟨2,2,3⟩` at 7 |
| `ctest -R binary_encoding` | with a cube supplied, no clause mentions term 0 beside another term, which in this encoding is exactly "term 0 is unordered" |

The second is labelled `slow` with a 900 s timeout. **All passed on 2026-08-16, the
second five for five in 65.4 s.** It is the only test that links both sides of this
boundary, because the question needs both.

**The ordering-on rows are the ones that matter and they were missing until the
merge.** The test called `encode_binary_rank_at_most(tensor, products)` with
`break_symmetry` false, so it compared cubes against a formula carrying no
ordering: silent about exactly the conjunction this file exists to keep apart. A
cube run now encodes with `first_term_pinned` and the whole run without it, and the
numbering still matches because that flag only skips an ordering whose auxiliary
variables come after every operand variable.

**Which maps the cubes even apply to**, because it is easy to assume more. These
representatives are `⟨n,m,k⟩`'s orbits and nothing else, so `orbit_cubes`
*refuses* every fixture that is not that product, and the known-rank fixtures in
`evidence/fixtures/` are polynomial and field multiplication maps. Only `matmul_2x2x2`
(rank exactly 7, decided here) and `matmul_2x2x3` (rank 11 published, `≥ 9` here)
are cube-validatable today. On `f2_5x5`, `f3_3x6`, `f2_3x8` and `f2_4x7`, a
constrained run validates the **ordering** constraint alone; their ranks and how
far each is safe to quote are in
[`evidence/benchmark_tensors/README.md`](../../../../evidence/benchmark_tensors/).

This is what closed the last unchecked row of
[`methods/satisfiability/correctness.md`](../../../satisfiability/correctness.md), *"a cube split
is complete ... not checked here"*. The row needed the test to be in the same tree
as the claim, which it now is.
