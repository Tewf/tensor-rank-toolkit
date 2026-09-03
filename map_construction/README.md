# map_construction/

Where a tensor file comes from when you do not want to type one: `make-tensor`
writes the tensor for a named problem — matrix multiplication, polynomial
multiplication, cyclic convolution, multiplication in a small field extension —
so every other tool can be pointed at it.

```sh
make-tensor --matmul 2 2 2 2 > my.tensor
```

The four spellings and what refuses an impossible size:
[`../OPTIONS/building-maps.md`](../OPTIONS/building-maps.md). Reading a
published algorithm in instead:
[`../formats/interchange/bringing-an-algorithm-in.md`](../formats/interchange/bringing-an-algorithm-in.md).
