# Which measured constants are facts, and which are facts about this laptop

Every number in [`../device.cpp`](../device.cpp) and
[`../../cli/tunables.h`](../../cli/tunables.h) was measured on **an RTX 4060
Laptop and one core of an i5-12450H with 16 GB**. That is stated everywhere it
should be. What was not stated is which of them stay true off that machine.

**A fitted number that ships as a constant is a bug on someone else's
hardware**, so the sort is worth doing outright.

- **Genuine**: a fact about the arithmetic, the format, or a CUDA limit. The
  same number is right on every machine.
- **Policy**: a budget somebody chose. It is not a measurement and does not
  claim to be, so it cannot be wrong on new hardware, only unhelpful.
- **Fitted**: read off *this* chassis. It is right here and wrong elsewhere by
  however much the two machines differ.

## Fitted, and there were four. Two are now derived

Two remain in the table, and only the first is still fitted in the sense that
matters: its right value is a **ratio of two timings**, not a count the kernel
already knows, so nothing can read it off a machine without taking a stopwatch to
one. That is why `auto` is refused for it and `measure-leaf floor` exists
instead. The second was closed the same day it was found and stays here for what
it cost, not because it is open.

| number | where | what it is really a measurement of | what happens elsewhere |
|---|---|---|---|
| `device_launch_floor = 8192` | [`../device.cpp`](../device.cpp), `cli/tunables.h` | the card's 21–29 µs fixed launch cost divided by the host's 3.1–4.0 ns an element. **Both halves are hardware.** | A card with a cheaper launch, or a slower host, crosses over lower and this floor keeps work on the host that the card would have won. A faster host crosses over higher and this floor sends work to a card that loses it. Re-fit it with `measure-leaf floor` and set `device_launch_floor` in `tunables.conf`; the knob exists, only its default is fitted. |
| `CUDA_ARCHITECTURES "89"` | `gpu_leaf/CMakeLists.txt` | 8.9 **is the RTX 4060**. Nothing else. | This was the worst of the four. A build on an A100 (8.0), a 3090 (8.6), an H100 (9.0) or anything newer produced no cubin the device could run, every launch failed with `cudaErrorNoKernelImageForDevice`, `answered_or_the_host` caught it and the host answered, so the run was **silently several hundred times slower** with one line on stderr to say so. Now `native` where CMake can detect the card, and the value stands where a packager pins one. |

## Derived since 2026-08-22, and no longer fitted to anything

Both memory numbers were the same measurement of the same laptop: *this machine
has 16 GB*. They are now **an eighth of what the machine reports**, read by
[`../machine.h`](../machine.h), which follows the model the bottom of this page
names: `set_worker_count(0)` asks `hardware_concurrency()` and no number from any
chassis appears in it.

The physical figure is rounded **up** to a power of two before the eighth is
taken, because firmware and the kernel keep some of what a machine is sold with
and this one reports 15.3 GiB, so a fraction of the raw number is a fraction of
an arbitrary one. An eighth of 16 GiB is exactly the 2 GiB and the 2048 that were
written here before, so **every published refusal still names the same figure**
and the change is invisible on this machine and visible on any other: 512 MiB on
a 4 GB box, 64 GiB on a 512 GB server, clamped at both ends.

`sat_memory_megabytes = auto` in `tunables.conf` is that reading; a number pins
one instead. `--max-memory` still beats both. `show-limits` prints what a run
resolved and which of the three it came from.

What they were, for the record:

| number | what it was really a measurement of |
|---|---|
| `budget = 2 GiB` | the comment said it: *"leaves room on a 16 GB desktop for a browser and an editor"* |
| `sat_memory_megabytes = 2048` | the same 16 GB, divided by the worker count in `satisfiability/rank_question.cpp`. The division was always right; the dividend was this machine's |

## Measured on 2026-08-22, and left where it was on purpose

`plateau_state_budget = 200'000` was tagged *PROVISIONAL: never measured* here
and in `cli/tunables.h` until the crossing it bounds was priced. **It is
measured now**, and the pair is published in
[`../../flip_graph/results.json`](../../flip_graph/results.json), where
`reproduce/measure.py --check` re-derives both rows on every push: `⟨2,2,2⟩`
crosses to 7 at a **380-state budget**, visiting 386 subspaces, and stays at the
naive 8 at 370, visiting 376. The negative row is why the boundary itself is
checked and not just the success.

**The default is 526 times what that crossing needs, and it stays.** At 380 the
run is 0.018 s; at the default it walks 66 063 subspaces for the same 7 and
costs **4.56 s**, because the crossing keeps going after it has seen the best map
it will find. Lowering the default to 380 would nonetheless be fitting a constant
to a single point: `⟨2,2,3⟩` does not cross at a 2 000-state budget and nobody has
found what it needs, so one shape's answer is not a rule.
[`../../tunables.conf`](../../tunables.conf) carries that reasoning beside the
value, which is where a person changing it will be.

It belongs on this page because it is neither of the two kinds above. It is not
fitted (nothing about 380 is a fact about this chassis, since it is a count of
subspaces), and it is not policy either, now that a measurement exists for it.
It is a bound held above its measurement on purpose, and the day a second shape
is measured it can be chosen rather than guessed.

## Genuine: the same on any machine

| number | where | why it does not move |
|---|---|---|
| `bytes_per_matrix = 56 + 8 * entries` | `../memory_budget.cpp` | `sizeof` of the structures on any LP64 target, plus an allocator header. Givaro fixes the element at `int64_t`, so the 8 is the format and not the machine. |
| `longest_vector_listed = 20` | `exhaustive_search/gf2_leaf.cpp` | a ceiling on `2^length`, and the file says outright it is *"not a tuning knob"*. |
| `kWordCeiling = 4`, `kScanDimensionCeiling = 256`, `kWalkDimensionCeiling = 64` | `gpu_leaf/leaf_backend.cpp` | what the kernels' own `__constant__` arrays hold. Compile-time facts about the source. |
| `handles(): 4, 5, 9, 16` | `gpu_leaf/leaf_backend.cpp`, `span_ranks.cu` | the four shapes a kernel is instantiated at. A fact about the template list. |
| `kRowsPerLaunch = 2048` | `gpu_leaf/pool_scan.cu` | keeps the grid under CUDA's **65 535** second-dimension ceiling, which is a CUDA limit and not a card's. |
| `survivor_capacity = 1 << 16`, `kHalvingsAllowed = 6` | `gpu_leaf/leaf_backend.cpp` | 512 KB of device buffer, with a documented halving fallback if it overflows. Sized against the answer, not against the card. |
| `maximum_pinned_order() = 8` | `matrix_sparsification/pattern_feasibility.cpp` | a ceiling on `Θ(order!)`. |
| `budget = 10'000'000` variables | `satisfiability/binary_encoding.cpp` | a ceiling on the encoding, in variables. |

## Policy: chosen, not measured

`search_node_limit = 5'000'000`, `search_leaf_limit = 100'000'000`,
`ilp_node_limit = 200'000`, `sat_timeout_seconds = 300`,
`ilp_time_limit_seconds = 300`, `workers = 1`,
`sat_solver_order`, `ilp_backend_order`, `device_order`.

These bound work or rank capabilities. `MEASURING.md` leans on two of them as
reproducibility anchors (a published node count above 5 000 000 means a
non-default flag was given), so moving a default would move a published table,
which is why none of them moved here.

## The one that is adaptive already, and is the model for the rest

`set_worker_count(0)` asks `std::thread::hardware_concurrency()`, and
`descent_search/minimise_rank.cpp` sizes its prefetch window as
`worker_count() * 4`. **No number from this chassis appears in either.** A
128-core machine gets 128 workers and a 512-deep window without anybody
re-measuring anything. That is what the other three axes should look like and
mostly do not.

## What is *not* asked, and deliberately

Nothing anywhere reads `cudaDeviceProp` to size a launch. `kThreadsPerBlock =
256` is the same on all three kernels and on all hardware. That is a defensible
default rather than a fitted number (256 is legal on every architecture CUDA
has shipped and within a factor of two of optimal on most), but it is a default
and not a tuning, and this page would rather say so than let a reader assume the
launch geometry was fitted to anything.
