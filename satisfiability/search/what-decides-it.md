# What decides it, which is not the schedule

Descending from a *tight* upper bound hits the floor exactly. Given `U = 9`,
asking 9 then 8 costs 108.461 s, the floor to three decimals, and nothing
starting from the naive ceiling of 16 matches it.

**So the lever is the upper bound, not the order of questions.** A heuristic
supplying a good `U` saves more than every schedule here put together. That is
why `find_rank` takes bounds rather than discovering them, and why the staged
design on `main` puts the heuristic between the flattening bound and this
decider.

Ascending stays the default: optimal when the bound is tight, within 4% of the
floor when it is not, and the only schedule that never reads the ceiling, so a
loose one cannot mislead it.

**Measured since, and it changes who the lever helps.** The paragraph above was
written when the floor on GF(16) was 4, five short of the rank. `rank_lower_bound`
now returns 8, and on all seven fixtures the floor is the rank or one below it,
so ascending already asks the two mandatory questions and no ceiling can save a
third. Buying `descend_from_ceiling`'s bracket to supply one costs 1.25x to 2.9x
the whole search: [`handing-over-the-bracket.md`](handing-over-the-bracket.md)
prices both ways on all seven. A good `U` is still the lever wherever the floor
is loose; these seven are no longer that case.

## Choosing probes by their timing, and why it is not here

Solve time peaks just below the rank, so a secant search could in principle
extrapolate to the rank from timings. It is rejected, for reasons that are
structural rather than a matter of taste:
[`search-by-timing.md`](../search-by-timing.md).
