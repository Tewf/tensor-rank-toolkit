# Why our result is a contrast with it, not a confirmation

On GF(16) bisection makes the fewest calls and finishes **last**, 113.614 s
against 110.094 s for the schedule that wins. That disagrees with the survey, and
the reason is ours to give rather than the survey's to have missed.

| GF(16) | Seconds |
|---|---|
| Bisection (fewest calls, finishes last) | 113.614 |
| Gallop down (wins) | 110.094 |

Source: [`the-five-schedules.md`](../search/the-five-schedules.md).

The exponential-versus-linear gap is in the *size of the cost range*, which for
MaxSAT is exponential in the number of soft clauses. Here the range is
`[flattening bound, n₁n₂]`, a dozen values at most, so log versus linear is a
handful of calls either way. **The asymptotic advantage has no room to act on a
range this narrow**, and what remains is dominated by which questions a schedule
happens to ask. That is why the whole choice is worth about 3% here, and the
survey does not say it because nobody had a range this small.

One nuance, since "nobody bisects" would be too strong: no solver bisects as its
*primary* schedule, but UWrMaxSat bisects as a second-phase fallback when
core-guided search stalls, and the 2024 weighted runner-up at 442 solved was
configured that way, with `-no-bin` absent. Its author enables it for weighted
instances and disables it for unweighted ones.
