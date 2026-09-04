# Positionnement, stated so it can be contradicted

**Not new**: the three schedules, the call-count pricing, the hybrid instinct.
Nor is the observation that bisection can lose on time despite winning on calls,
which is `[heras2011]`'s about core-guided binary search.

**New, as far as reading found**: the per-question price table in
[`bracket/`](../bracket/), which on GF(16) prices all thirteen questions
from k = 4 to k = 16 at once:

| GF(16), from [`every-question-priced.md`](../bracket/every-question-priced.md) | Seconds |
|---|---|
| k = 4 to k = 7 (retired now, priced anyway) | 4.10 total |
| k = 8 (the refusal that decides it) | 108.2 |

Every question here is a separate deterministic
process, so its cost is independent of the order it is reached in, and pricing
all of them prices every schedule exactly and at once, including unimplemented
ones. **A MaxSAT solver cannot do this**, because it is incremental: a call's
cost there depends on what the solver learned in the calls before it. The
no-linking rule that costs this module incrementality is what buys it exact
schedule pricing. That is a trade, not a gap.

**The baseline** for this strand is therefore `[morgado2013]`'s taxonomy and the
MaxSAT Evaluation record beside it, and the review is finished because that
baseline can now be named. On the other side of the module, a refutation is
measured against `[wang2026]`:
[`../../../writeup/the-research-front/refutation-baseline.md`](../../../writeup/the-research-front/refutation-baseline.md).
