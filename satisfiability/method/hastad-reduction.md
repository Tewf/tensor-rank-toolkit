# Håstad's reduction

`(2 + n + 2m) × 3n × (3n + m)`, built in `Θ(n·m)` entries, with target rank
`4n + 2m`. The witness from a satisfying assignment costs one rank-one
decomposition per clause and is otherwise arithmetic.

It is a reduction, not a way to solve SAT. A formula of ten variables and
twenty clauses becomes a `52 × 30 × 50` tensor asked for rank 80, which is
enormously harder than the formula it came from. That is what a hardness proof
looks like from the inside, and it is the reason the arrow that gets used in
practice points the other way.

```
n = 10, m = 20
shape:  (2 + n + 2m) x 3n x (3n + m)  =  52 x 30 x 50
target: 4n + 2m                       =  80
```
