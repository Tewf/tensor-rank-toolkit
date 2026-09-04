# Why the tree wins, and where it stops

Not luck. Pinning **raises the dimension the tree starts from**, so it deletes a
level. `span(T) + t` has dimension 5 at `⟨2,2,2⟩` against a target of 6, so one level
remains and the 45 to 72 nodes reported are the entire search. Pinning does nothing
comparable to a CNF instance's difficulty, which is why the solver route gains
nothing from it.

The advantage therefore scales with `k - dim(span(T) + t)`, and it is gone as soon as
that is large. Measured: at `⟨3,3,3⟩` with `k = 23` the depth is 13, and the tree
returned no verdict on any of the thirteen candidates in **723 s** before it was
stopped, against a 5 000 000 node budget per candidate. This is a result about shallow
pinning near the rank, not about matrix multiplication in general.

The shallow case, where the tree does win, is the command run in
[`README.md`](README.md): `⟨2,2,2⟩` at `k = 6`, one level deep, 45 to 72 nodes.
