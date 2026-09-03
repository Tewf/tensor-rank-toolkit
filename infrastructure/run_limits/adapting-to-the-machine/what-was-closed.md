# What the audit closed, what it did not, and why the second list is short

[`the-audit.md`](the-audit.md) is the state before any of this. This page is the
diff, in the order [`README.md`](README.md) argues for: cores and memory first,
because they help every user, and the card last, because it helps almost none.

**No default moved and no published number changed.** Every flag added here
defaults to what the compiled default already was, so a run that gives none of
them behaves exactly as it did. `ctest` is 70 and 11 where it was 68 and 11, and
the two new ones are the two written here.

## (a) Cores: two `parallel_for`s nobody could reach, and one that aborted

**`decide-rank-by-sat` gained `--threads`.** `methods/satisfiability/rank_question.cpp`
has run its cubes through `parallel_for` since cube-and-conquer arrived, with the
measurement in the comment beside it: five cubes of `matmul_2x2x2 --target 6`,
3.36 s sequentially against 0.982 s together, **3.42x**. Nothing on that command
line reached `set_worker_count`, so `worker_count()` was 1 for every run of the
tool and the split ran one cube at a time. The flag is the whole fix; the loop was
already there, already correct, and already documented.

**`lower-the-bound` gained `--threads`, and a loop for it to reach.** The
incumbent search costs one `minimum_weight_basis_with` per surviving move at every
node. The identical call `descent_search` has spread over cores for months. The
node loop above it cannot be threaded (the incumbent decides what the next node
prunes), and neither can the filter that decides which moves are children (a
`std::set` of residues, where offering order decides which of two equal residues
survives). So the two are separated: **the filter stays sequential and the costing
goes to the workers, each writing its own slot.** The children are then the same
children in the same order at any thread count, which
`check_the_limits_reach_the_commands.sh` asserts on `matmul_2x2x3`: 341 nodes,
159 860 children, identical at 1 and at 4.

**`contraction_ranks` gained one.** The rank-sum floor is up to 2²⁰ independent
ranks, each built from its own index, and three commands offering `--threads`
stopped at its door. The one-worker path is byte for byte what it was, scratch
buffer and all; the parallel path gives each item its own coefficients.

**The plateau crossing keeps its ✖, and the audit's "worse than absent" is priced
rather than repeated.** `--plateau 3` on `⟨2,2,2⟩` costs **20.67 s at one worker,
20.85 s at four and 20.84 s at twelve**, identical counts throughout. The only
loop that could take workers is one `minimum_weight_basis_with` per candidate per
state, the same call `descent_search` already spreads, but that run is 200 003
states at **103 µs each** and `parallel_for` creates and joins its workers on
every call, **87 µs for seven of them** here. One call a state would add most of a
state to every state. Spreading it wants a persistent pool, a primitive nothing
else here needs and one that would move the cost profile every other strand was
measured under, so what is written down instead is the limitation: at the flag in
`minimise-rank --help`, in
[`../../OPTIONS/searching-for-rank.md`](../../../OPTIONS/searching-for-rank.md) and
in the console's catalogue. The flag is not refused, because steps 1 to 3 above
the crossing do read it.

**And `parallel_for` now carries a worker's exception out.** This is the one that
was a live production bug rather than a missed opportunity: an exception leaving a
`std::thread`'s function is `std::terminate`, so **every `require_room` refusal
raised under `--threads 2` or more was a SIGABRT instead of the sentence naming
the number.** `require_room` exists precisely so that a machine smaller than
the one a run was written on says what it cannot afford rather than dying. The one
graceful failure here was the one that aborted, and only when threads were asked
for. `test_parallel.cpp` asserts it at one worker and at four; with the fix
reverted that test does not fail, it *aborts*, at exit 134.

## (b) Memory: five allocations that had nothing in front of them

Each is exponential or cubic in a number read off a command line or a tensor
file, each was a `reserve` or a `resize` straight to the allocator, and each now
prices itself first. A refusal naming the number is a result; an out-of-memory
kill is not.

| where | what it asked for | reachable by |
|---|---|---|
| `methods/bilinear_rank/branch_and_bound/level_lowering_moves.cpp` | `p^r` vectors | `lower-the-bound --summand-rank r`, which took any count |
| `methods/curve_bounds/interpolation_programme.cpp` | an `O(degree²)` frontier, twice | `curve-bounds --degree`, which took any whole number: 100 000 asks for about 240 GB |
| `methods/pencil_rank/sumi_bound.cpp` | `x^p - x`, linear in `p` | **the tensor file**, whose header is checked for primality and not for size: `field 2147483647` asks for 17 GB |
| `methods/matrix_sparsification/combinations.cpp` | `C(total, size)` subsets | `sparsify-operator` on anything larger than the 7x4 operators shipped: `C(47, 23)` ≈ 1.6e13 on a `⟨4,4,4⟩` operator, which `minimise-rank --emit-operators` invites |
| `methods/bilinear_rank/map_construction/map_construction.cpp` | `(rows·inner·columns)²` entries, and `length³` | `make-tensor --matmul 2 100 100 100`, which is 7.2 TiB |

Each count is computed with its own overflow checked, and where the product of
two dimensions is itself the multiplication that wraps, the check is asked as one
row and then as a count of rows rather than as one product.

**Five commands gained `--max-memory`, which their own refusals already named.**
`require_room` ends every refusal with *"Raise it with --max-memory if the machine
has the room"*, and `lower-the-bound`, `walk-scheme`, `curve-bounds`,
`sparsify-operator`, `decide-rank-by-pencil` and `make-tensor` did not have the
flag. A command that prints that sentence and then refuses the flag is worse than
one that never mentions it.

**Three guards were added, failed, and were reverted.** They are in
[`the-audit.md`](the-audit.md) under "the knob that means two things", and they
are the most useful thing this audit learned: `--max-memory` selects the cheap
representation as well as capping the expensive one, so the cheap one's own cost
cannot be priced against it.

## (c) The card: one line, and it was the worst bug of the three axes

`infrastructure/gpu_leaf/CMakeLists.txt` said `CUDA_ARCHITECTURES "89"`. **8.9 is the RTX 4060
and nothing else.** On any other card the fatbin held no image the device could
run, every launch returned `cudaErrorNoKernelImageForDevice`, `leaf_backend.cpp`
caught it exactly as designed and let the host answer, so the run was several
hundred times slower with one line on stderr to say so, and the two seams this
whole directory exists for were dead. It now asks CMake for `native`, which asks
the driver what is really in the machine; an explicit `CMAKE_CUDA_ARCHITECTURES`
still wins, which is what a packager cross-compiling needs.

**No third seam was built, and that is the finding rather than a deferral.** The
two that exist cover both GPU-shaped loops in the repository. The one honest
candidate, `contraction_ranks`, is the general-field path, and `gpu_leaf`'s own
README already says that field has none of the bit arithmetic the kernels are made
of; its whole cost is under half a second, once, before a search starts. It got
cores instead, which work on every machine and needed no kernel.

**No strand decides its own device, so nothing had to be routed through
`chosen_device`.** It is called from four places and three of them are the two
seams; the fourth is the plan reporting what those three will do.
