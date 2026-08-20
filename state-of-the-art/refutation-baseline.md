# The baseline a refutation here is measured against

**`[wang2026]`**, March 2026, and it is the front on the infeasible side. Chengu
Wang classifies the orbits of constraint subspaces under a group of
rank-preserving symmetries acting on one argument, runs a dynamic program over
the orbits combining four lower-bound techniques, and emits a certificate a
separate verifier rechecks. **It raises `⟨3,3,3⟩` over F₂ from 19 to 20**,
retiring `[blaser2003]`'s bound after twenty-three years, improves three more
small formats, and adds eighteen bounds for **polynomial multiplication**, which
is what every fixture here is.

**It is implemented and public**, MIT-licensed C++ under the author's own name at
`github.com/wcgbg/tensor-rank-lower-bound`, so nothing here is unimplemented
ground. The paper reports the `⟨3,3,3⟩` proof found "in about 40 minutes on a
laptop" with the certificate verifying "in seconds"; **it publishes no
per-instance timing table, so any sharper verification figure is not a quotable
number** and this file does not invent one.

**Where that leaves this strand, stated plainly.** The shape is the same and the
reach is not. Both search and then hand a refusal to an independent checker, and
that discipline is the one place the two are level: a DRAT proof rechecked by
`drat-trim` is exactly Wang's certificate argument in a different notation. But
Wang settles `⟨3,3,3⟩`, and the largest thing this encoding refutes is far
smaller: `f3_3x6` does not answer at ten in 300 s, though the exhaustive search
settles that map at nine in under eight seconds, and `f2_5x5` is only bracketed
at 13 ≤ rank ≤ 14, where `[bdez2012]` settled 13 by exhaustive search in 2012.
**The gap is not the certificate, it is the orbit
classification and the dynamic program in front of it**: a monolithic CNF asks
one enormous question where Wang asks many small ones and combines them. That is
the same lesson the cube work reaches from the other end.
