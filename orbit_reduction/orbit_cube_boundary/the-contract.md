# What `orbit_cubes` hands over, and in what coordinates

## What is supplied

[`orbit_cubes.h`](../orbit_cubes.h):

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
