# The fixed-k finder against the heuristics

| fixture | known | `minimise-rank` | `walk-scheme` | `find-at-rank --descend` |
|---|---|---|---|---|
| `⟨2,2,2⟩` GF(2) | 7 | 8, 0.01 s | **7, 0.11 s** | 7, 42.9 s |
| `gf16` | 9 | **9, 0.04 s** | 9, 0.69 s | 9, 61.3 s |
| `f2_5x5` | 12 to 14 | **14, 7.2 s** | **14, 10.8 s** | 15, 89.2 s |
| `f3_3x6` GF(3) | 10 | **10, 18.9 s** | 12, 10.9 s | not found at 10, 300 s |

**The finder is dominated on every fixture.** It reaches the known answer on
`⟨2,2,2⟩` and `gf16` and is between 390 and 1500 times slower doing it; it returns a
*worse* bound on `f2_5x5`, 15 against 14; and on `f3_3x6` it does not reach 10 at all
in 300 s while the greedy returns a verified 10-product algorithm in 18.9 s.
