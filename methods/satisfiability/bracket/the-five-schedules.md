# The five schedules, summed from that table

Seconds. `floor` is the mandatory two questions; gallop up and gallop down are
named for the literature's galloping search, exponentially growing steps from
the floor and from the ceiling respectively, then bisection of the interval the
last step overshot.

| fixture | floor | ascending | descending | bisection | gallop up | gallop down |
|---|---|---|---|---|---|---|
| f2_2x2 | 0.006 | **0.006** | 0.015 | **0.006** | **0.006** | 0.015 |
| f2_2x3 | 0.026 | **0.026** | 0.040 | **0.026** | **0.026** | 0.040 |
| gf4 | 0.006 | **0.006** | 0.011 | **0.006** | **0.006** | 0.011 |
| gf8 | 0.085 | 0.112 | 0.142 | **0.104** | 0.128 | 0.142 |
| w_state | 0.009 | **0.009** | 0.012 | **0.009** | **0.009** | 0.012 |
| matmul_2x2x2 | 0.495 | **0.620** | 1.044 | 0.716 | 0.760 | 0.865 |
| **GF(16)** | 108.461 | 112.533 | 110.421 | 113.614 | 110.399 | **110.094** |

**Ascending wins on the cheap fixtures and loses on the only expensive one**,
where it comes fourth of five. It ties the floor wherever the bound already
equals the rank, four of these seven, because it then asks one question and
stops. It lost on GF(16) because the floor was five short there, so it walked
through `k = 7` at 3.7 s: the second dearest question in the table, and one no
other schedule asks.

**That specific handicap is gone.** The floor on GF(16) is now 8 rather than 4,
one short of the rank instead of five, so ascending asks `k = 8` and `k = 9` and
nothing else. The table above was measured before that and is not re-run here;
what can be said without re-running it is that the four questions ascending was
paying for and no other schedule asked, 4.10 s of its 108.461 s, are no longer
asked by anyone. The comparison between schedules should be re-measured before it
is quoted again.
