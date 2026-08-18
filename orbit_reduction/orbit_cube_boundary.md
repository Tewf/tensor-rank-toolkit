# The orbit cube boundary

Two modules meet here and this file is the whole contract. `orbit_reduction/`
supplies representatives; `satisfiability/` consumes them. Written while they were
on separate branches and kept after the merge, because they are still compiled
apart and the contract is still what holds them together.

Both sides break the symmetry of the same formula. Theirs does it with a term
*ordering*; this one does it with the map's full automorphism group. Each is
sound alone. **Their conjunction is not**, and that is the one thing below that
cannot be got wrong.

## What is supplied

[`orbit_cubes.h`](orbit_cubes.h):

```cpp
std::vector<std::vector<int>> orbit_cubes(
    const Field& field, const std::vector<Matrix>& slices,
    std::size_t rows, std::size_t inner, std::size_t columns,
    const std::vector<int>& left_variables,
    const std::vector<int>& right_variables);
```

One cube per orbit, each a list of literals over the **consumer's** variable
numbers. Together they cover every possible first term up to the group, so
solving the formula once per cube decides the same question as solving it whole.
The union is *unknown*, never *no*, if any cube went unanswered: a cube that
gave up has refuted nothing.

Linked from the command and not from the `satisfiability` library:
`target_link_libraries(decide-rank-by-sat PRIVATE satisfiability orbit_reduction ...)`.
The command is the common ancestor of the two modules, so the library layering
stays one-directional, and a static archive contributes only the objects
referenced.

## What a representative is

A rank-one term of `⟨n,m,k⟩` is a pair `(U, V)`, `U` an `n×m` matrix and `V` an
`m×k` one. The stabiliser acts by change of basis on each side, sharing the
middle, and by Covanov's Corollary 18 an orbit is fixed by exactly three
numbers: `rank U`, `rank V`, and `rank UV`, the last confined to
`max(0, rU+rV−m) ≤ t ≤ min(rU, rV)`. So the list is a triple loop over
`rU ≥ 1`, `rV ≥ 1`, `t`. No group is built and nothing is enumerated: 5
representatives for `⟨2,2,2⟩`, 13 for `⟨3,3,3⟩`, 26 for `⟨4,4,4⟩`, against
261 121 and 4 294 836 225 first terms.

Why `rU ≥ 1, rV ≥ 1` loses nothing: the group carries any decomposition of `T`
to another, and the terms are a set, so a decomposition can be permuted to put a
term with all three components nonzero at position 0, then moved until that term
is its orbit's representative. The only map with no such term is `T = 0`.

## The layout

Pass the whole variable arrays, not a slice of them. The offset arithmetic lives
on the supplying side so that no consumer repeats it:

    left_variables [term * rows  * inner   + coordinate]
    right_variables[term * inner * columns + coordinate]

which is the layout `satisfiability/binary_encoding.h` already builds. A positive
literal asserts the coordinate is one, a negative literal that it is zero: the
DIMACS convention, so the encoder needs to hand over no header to honour it.

**The word `rows` does two jobs here, and one reader has already been caught by
it.** The encoder's `rows` is the tensor's row count and says `left[l * rows + i]`;
this module's `rows` is the first of the matmul dimensions `⟨rows, inner,
columns⟩`. For a matrix multiplication tensor those are the same number, since a
slice of `⟨2,2,2⟩` is 4x4 and `2*2 = 4`, so the two formulas above agree with the
encoder's despite looking different. The reason they must is spelled out where the
array is defined, in `binary_encoding.h`; the reason it matters is that a consumer
who took the names at face value would build a different layout and get a wrong
answer rather than an error, which is what `first_term` refusing a length that is
not a whole number of terms exists to catch.

`slices` is the tensor the consumer encoded, and the shape is **checked against
`⟨rows, inner, columns⟩` rather than taken on trust**. The representatives are
written for that map in that coordinate order, so naming the wrong shape would
pin the first term to a map the tensor does not contain, and refute a
decomposition that exists. That is why the shape is an argument and not a comment.

## The rule that makes the conjunction sound

**A cube pins the first term and orders nothing.** A representative is not
obliged to be lexicographically least, so the consumer's own term ordering
**must skip term zero**. Conjoin an unmodified ordering with a cube and the two
together exclude decompositions that exist, which reads as a refutation.

Second guard, from `orbit_plan.md`: orbit pruning requires the pool to be closed
under the action. `all_rank_one_maps` is closed; `rank_one_candidates` is not.
Refuse rather than assume.

## The hazard, stated plainly

A wrong or incomplete symmetry constraint turns a satisfiable formula
unsatisfiable **silently**. That is a false lower bound, and nothing downstream
catches it: a *yes* carries its own proof and gets multiplied out against the
map, but a *no* rests on the search having been complete.

An incomplete group only costs speed. Fewer verified elements means more orbits
means a bigger search, still exhaustive and still sound. Only an element that was
never verified, or a cube set that misses a first term, can corrupt an answer.

## Validation, which is the deliverable

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
`fixtures/` are polynomial and field multiplication maps. Only `matmul_2x2x2`
(rank exactly 7, decided here) and `matmul_2x2x3` (rank 11 published, `≥ 9` here)
are cube-validatable today. On `f2_5x5`, `f3_3x6`, `f2_3x8` and `f2_4x7`, a
constrained run validates the **ordering** constraint alone; their ranks and how
far each is safe to quote are in [`known_ranks.md`](../descent_search/known_ranks.md).

This is what closed the last unchecked row of
[`satisfiability/correctness.md`](../satisfiability/correctness.md), *"a cube split
is complete ... not checked here"*. The row needed the test to be in the same tree
as the claim, which it now is.
