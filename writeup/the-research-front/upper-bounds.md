# Upper bounds: the front moved twice, and neither time by exhaustive search

**`[alphatensor2022]`** put reinforcement learning on it. AlphaZero treated
decomposition as a single-player game and produced 14 236 non-equivalent schemes
for `⟨4,4,4⟩` alone, the first time a learned system improved on human schemes.

**`[kauers2023]`** then matched much of that with no learning at all. The **flip
graph** starts from a decomposition that works and rewrites it: a *flip* swaps a
shared factor between two terms and keeps the sum, so rank is unchanged and the
walk can move sideways for ever, and a *reduction* fires when two terms come to
share two factors, dropping the rank by one. A random walk on that graph found
`⟨5,5,5⟩` in 95.

**`[moosbauer2025]`** added the tensor's own symmetries to the walk and reached
**`⟨5,5,5⟩` in 93 and `⟨6,6,6⟩` in 153**, and `[kauers2025]` generalised the
construction again.

**`[alphaevolve2025]`** then found **`⟨4,4,4⟩` in 48 multiplications over `ℂ`**,
the first improvement on Strassen applied twice, 49, in fifty-six years. That
one needed complex coefficients, which cost arithmetic, so the follow-up work is
about removing them: `[moran2026]` gives a systematic method that either
converts a complex scheme to a rational one or proves no rational equivalent
exists, generalising Dumas, Pernet and Sedoglavic's earlier ad hoc results.

**The pattern is that every recent record came from walking or evolving a
decomposition that already worked, not from searching a space from nothing.**
