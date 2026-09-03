# What the 2026-08-20 rewrites were worth, end to end

The leaf was rewritten three times in a day and the canonical route four. This
is what that came to on whole questions rather than on per-element rates, taken
under [`../../MEASURING.md`](../../MEASURING.md): one core, fastest of three, the lock
held, load below 1.0 at the start of each set.

**Every node count below is unchanged.** That is the first thing to check and
the only one that would matter if it failed: the tool answers what it answered.

## The leaf, per element

Same question, same shape, `⟨4,4,4⟩` at the dimension each route is chosen at:

| route | before | after | |
|---|---|---|---|
| pool scan | 940.2 ns | **3.3 ns** | **285x** |
| subspace walk | 62.6 ns | **3.9 ns** | **16x** |

Three changes stack there: forming the element in words rather than through
Givaro, then never forming it at all on the scan, then asking whether the rank
is one instead of computing it.

## Whole questions

| question | nodes | before | after | |
|---|---|---|---|---|
| `f3_3x6 --target 9` | 4 729 | 7.65 s | **3.93 s** | **1.95x** |
| `matmul_3x3x3 --target 23 --node-limit 300` | 300 | 3.55 s | **0.313 s** | **≥11.3x** |
| `matmul_2x2x2 --target 6` | 25 399 | 0.0362 s | 0.0284 s | control |
| `matmul_2x2x2 --target 7` | 7 436 | 0.0214 s | 0.0125 s | control |

`f3_3x6` is the general-field path and the cleanest reading: an odd
characteristic, no bit packing anywhere, 1.95x from the Gray walk and
`is_rank_one` alone. The `matmul_3x3x3` row was taken at load 1.37, so it is a
**lower bound**: load can only slow a run, and it came back faster than
published. The last two rows are milliseconds, which `MEASURING.md` counts as
correctness controls rather than speed evidence.

## The card, now that it is routed

`matmul_3x3x3 --target 23 --node-limit 300`, same 300 nodes either way:

| | seconds |
|---|---|
| `--device cpu` | 0.295 |
| `--device auto`, engaging the card | **0.160** |

The launch floor is measured at 8 192 elements, and the run says why it chose
what it chose: *"gpu (261121 elements at the deepest leaf, at or over the 8192
launch floor)"*.

## The canonical route

| | before | after |
|---|---|---|
| `matmul_2x2x2` sweep, canonical | 0.263 s | **0.0576 s** |
| the plain route it must beat | 0.0102 s | **0.00713 s** |
| the gap | 25.8x | **8.1x** |

Two thirds of the gap closed, and it still loses: at this shape it cannot win,
since the parent test costs more per node than a 53x node saving is worth. What
moved is where the crossover sits, and that
[`../../methods/canonical_factorisation/canonical-augmentation.md`](../../methods/canonical_factorisation/canonical-augmentation.md)
now states without a fitted intercept that was never there.
