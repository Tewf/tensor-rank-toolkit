# Two corrections, since both wrong answers were published

**Gallop was made the default on one contended instance**, reported 2.3x faster
than ascending. The true difference is under 2% either way.

| Claimed | Measured |
|---|---|
| gallop 2.3x faster than ascending | under 2% either way |

**Then ascending was published as the winner**, measured end to end while five
stray `cbc` processes held the machine, and generalised from fixtures costing
milliseconds. Ranking millisecond measurements was the error both times.
