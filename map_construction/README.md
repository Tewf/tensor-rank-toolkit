# map_construction/

Where a tensor file comes from when you do not want to type one: `make-tensor`
writes the tensor for a named problem, so every other tool can be pointed at
it.

In this folder:

- [`commands/`](commands/): `make-tensor`, with four spellings: `--matmul`
  for matrix multiplication, `--polynomial` for polynomial multiplication,
  `--cyclic` for cyclic convolution, `--field` for multiplication in a small
  field extension.
- [`map_construction.h`](map_construction.h): the constructions themselves,
  which the tests and other modules call without going through the command.
- [`tests/`](tests/): each construction checked against the definition of
  the map it claims to build.

Example of use:

```sh
make-tensor --polynomial 2 2 2 > my.tensor
head -4 my.tensor
# Polynomial multiplication of 2 coefficients by 2, over GF(2).
# Naive cost 4 multiplications, written by make-tensor.
# field 2
# shape 3 2 2
```

The four spellings and what refuses an impossible size:
[`../OPTIONS/building-maps.md`](../OPTIONS/building-maps.md). Reading a
published algorithm in instead:
[`../formats/interchange/bringing-an-algorithm-in.md`](../formats/interchange/bringing-an-algorithm-in.md).
