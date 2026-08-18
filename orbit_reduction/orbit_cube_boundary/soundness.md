# What keeps the two symmetry breakings sound together

## The rule that makes the conjunction sound

**A cube pins the first term and orders nothing.** A representative is not
obliged to be lexicographically least, so the consumer's own term ordering
**must skip term zero**. Conjoin an unmodified ordering with a cube and the two
together exclude decompositions that exist, which reads as a refutation.

Second guard, from [`../orbit_plan/`](../orbit_plan/README.md): orbit pruning
requires the pool to be closed under the action. `all_rank_one_maps` is closed; `rank_one_candidates` is not.
Refuse rather than assume.

## The hazard, stated plainly

A wrong or incomplete symmetry constraint turns a satisfiable formula
unsatisfiable **silently**. That is a false lower bound, and nothing downstream
catches it: a *yes* carries its own proof and gets multiplied out against the
map, but a *no* rests on the search having been complete.

An incomplete group only costs speed. Fewer verified elements means more orbits
means a bigger search, still exhaustive and still sound. Only an element that was
never verified, or a cube set that misses a first term, can corrupt an answer.
