# The orbit cube boundary

Two modules meet here and this folder is the whole contract. `orbit_reduction/`
supplies representatives; `satisfiability/` consumes them. Written while they were
on separate branches and kept after the merge, because they are still compiled
apart and the contract is still what holds them together.

Both sides break the symmetry of the same formula. Theirs does it with a term
*ordering*; this one does it with the map's full automorphism group. Each is
sound alone. **Their conjunction is not**, and that is the one thing here that
cannot be got wrong.

| | |
|---|---|
| [`the-contract.md`](the-contract.md) | what `orbit_cubes` hands over, what a representative is, and the array layout it is written in |
| [`soundness.md`](soundness.md) | why a cube and a term ordering must not both constrain term zero, and what a wrong constraint costs |
| [`validation.md`](validation.md) | the three tests no refutation built on cubes is believed without |
