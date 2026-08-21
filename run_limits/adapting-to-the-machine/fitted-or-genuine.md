# Which measured constants are facts, and which are facts about this laptop

Every number in [`../device.cpp`](../device.cpp) and
[`../../cli/tunables.h`](../../cli/tunables.h) was measured on **an RTX 4060
Laptop and one core of an i5-12450H with 16 GB**. That is stated everywhere it
should be. What was not stated is which of them stay true off that machine.

**A fitted number that ships as a constant is a bug on someone else's
hardware**, so the sort is worth doing outright.

- **Genuine** — a fact about the arithmetic, the format, or a CUDA limit. The
  same number is right on every machine.
- **Policy** — a budget somebody chose. It is not a measurement and does not
  claim to be, so it cannot be wrong on new hardware, only unhelpful.
- **Fitted** — read off *this* chassis. It is right here and wrong elsewhere by
  however much the two machines differ.

## Fitted, and there are four

| number | where | what it is really a measurement of | what happens elsewhere |
|---|---|---|---|
| `device_launch_floor = 8192` | [`../device.cpp`](../device.cpp), `cli/tunables.h` | the card's 21–29 µs fixed launch cost divided by the host's 3.1–4.0 ns an element. **Both halves are hardware.** | A card with a cheaper launch, or a slower host, crosses over lower and this floor keeps work on the host that the card would have won. A faster host crosses over higher and this floor sends work to a card that loses it. Re-fit it with `measure-leaf floor` and set `device_launch_floor` in `tunables.conf`; the knob exists, only its default is fitted. |
| `budget = 2 GiB` | [`../memory_budget.cpp`](../memory_budget.cpp) | the comment says it: *"leaves room on a 16 GB desktop for a browser and an editor"*. | On a 4 GB box 2 GiB is most of the machine and the refusal comes too late. On a 512 GB server it refuses runs that would have fitted forty times over. `--max-memory` moves it — **on the commands that have the flag**, which is now all of them. |
| `sat_memory_megabytes = 2048` | `cli/tunables.h` | the same 16 GB. `satisfiability/rank_question.cpp` divides it by the worker count with the arithmetic written out: *"twelve workers at 2 GiB each would be a 24 GiB ceiling on a 16 GB machine"*. | The division is right; the dividend is this machine's. On a 128 GB server it starves each solver for no reason, and on an 8 GB one twelve workers still overcommit. |
| `CUDA_ARCHITECTURES "89"` | `gpu_leaf/CMakeLists.txt` | 8.9 **is the RTX 4060**. Nothing else. | This was the worst of the four. A build on an A100 (8.0), a 3090 (8.6), an H100 (9.0) or anything newer produced no cubin the device could run, every launch failed with `cudaErrorNoKernelImageForDevice`, `answered_or_the_host` caught it and the host answered — so the run was **silently 500× slower** with one line on stderr to say so. Now `native` where CMake can detect the card, and the value stands where a packager pins one. |

## Neither, and the file says so

`plateau_state_budget = 200'000` is tagged **PROVISIONAL: never measured** in
`cli/tunables.h`. That is the right label: it is a guess with a flag on it, not a
number pretending to be evidence. It is listed here so nobody promotes it to
"measured" by reading it beside three numbers that were.

## Genuine — the same on any machine

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

## Policy — chosen, not measured

`search_node_limit = 5'000'000`, `search_leaf_limit = 100'000'000`,
`ilp_node_limit = 200'000`, `sat_timeout_seconds = 300`,
`ilp_time_limit_seconds = 300`, `workers = 1`,
`sat_solver_order`, `ilp_backend_order`, `device_order`.

These bound work or rank capabilities. `MEASURING.md` leans on two of them as
reproducibility anchors — a published node count above 5 000 000 means a
non-default flag was given — so moving a default would move a published table,
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
default rather than a fitted number — 256 is legal on every architecture CUDA
has shipped and within a factor of two of optimal on most — but it is a default
and not a tuning, and this page would rather say so than let a reader assume the
launch geometry was fitted to anything.
