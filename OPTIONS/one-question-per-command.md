# One question per command

**Audited 2026-08-21, when there were fourteen.** Grouped by the question a
reader arrives with rather than by the directory a command lives in, because
nobody chooses a tool by directory. What each flag costs is the tables beside
this page; this one is only about which line to type, and about why the list is
as long as it is.

The count was fourteen and not thirteen: `list-solvers` is the one command whose
source is not named `*_main.cpp`, so every list assembled by looking for that
pattern has missed it.

## The twelve, and what each is asked

| Command | The question no other command answers |
|---|---|
| `minimise-rank` | How few products does a cheap descent reach, and what did each of its three steps cost? |
| `lower-the-bound` | How much cheaper can a branch and bound make an algorithm I already hold? |
| `walk-scheme` | Can a decomposition that already works be moved sideways into a smaller one? |
| `decide-rank` | Is there an algorithm with `k` products, decided by a tree this repository walked itself? |
| `decide-rank-by-sat` | The same question, decided by somebody else's solver, with a refutation checkable as DRAT |
| `decide-rank-by-pencil` | Two slices: what does the Kronecker canonical form prove, and is that the rank or only a bound? |
| `factor-over-canonical-basis` | The rank as `S = C A`, so a reader who trusts nothing here can multiply it out |
| `deflate-strictly` | Is this committed candidate refutable, and what did waiting for the proof cost? |
| `enumerate-subspaces` | How many solution subspaces are there at `k`, and how many orbits do they fall into? |
| `sparsify-operator` | How few nonzero entries can one operator be written with? |
| `curve-bounds` | What does interpolation on an algebraic curve bound, and which backend is available to say so? |
| `make-tensor` | Build a map to run any of the others on |

## Three kinds of answer, which is what the grouping is for

**Decided.** `decide-rank`, `decide-rank-by-sat`, `decide-rank-by-pencil`,
`factor-over-canonical-basis` and `deflate-strictly` can say *no* and mean it.
**Found.** `minimise-rank`, `lower-the-bound` and `walk-scheme` refute nothing
ever: a spent budget hands back a weaker algorithm, never a bound.
**Counted.** `enumerate-subspaces` and `sparsify-operator` return a number about
the object rather than a verdict on it.

## Four merges that lose, and why

The rule they are measured against: **a merge that leaves one command carrying
two mutually exclusive flag sets is worse than two commands.**

- **`decide-rank` + `decide-rank-by-sat` + `decide-rank-by-pencil` → `--route`.**
  Refused. One question, three machines, but eight tree-only flags against
  thirteen solver-only ones, and `--max-memory` already means a pool budget in
  bytes on one and a solver cap in megabytes on the other. The pattern
  `factor-over-canonical-basis --route auto|exhaustive|sat|canonical` solves
  works because its routes differ in the *library* and share every flag; these
  do not. Pencil alone contributes no flags and so passes the flag test, and
  loses on two others: `decide_rank_main.cpp` is already the longest file here,
  and exit 3 would come to mean two things in one command — `check_exit_codes.sh`
  records that pencil is *the one place* Undecided means the mathematics stopped
  rather than a budget did.
- **`minimise-rank` + `lower-the-bound`.** Refused. Same question, two machines,
  and no name collides — but four descent-only flags against six
  incumbent-only ones, and `--json` and `--steps` would silently mean nothing on
  half the command.
- **`walk-scheme` into either of those.** Refused twice over: `--steps` already
  means pipeline stages on `minimise-rank` and flips here, and `--from` means a
  starting scheme size here and an enum on `lower-the-bound`.
- **`deflate-strictly` + `enumerate-subspaces`.** Refused. They share a directory
  and nothing else: one decides a candidate and exits 0, 1 or 3; the other
  counts and always exits 0.

## Two that are not tools, and leave the surface

The line is [`../MEASURING.md`](../MEASURING.md)'s own: **counts reproduce
anywhere and timings do not.** A command whose output is nanoseconds is an
instrument for this repository, not a question a reader brings about their map.

- **`price-canonical-route`** measures the four prices `canonical_route_price.h`
  predicts from, and its own header names `measure-leaf` as its model.
  `measure-leaf` sits outside `commands/` already; this now does too. Same
  binary, same path, off the tool list.
- **`list-solvers`** is now **`curve-bounds --solvers`**, beside the `--route`
  its answer is for and the `--table` that already prints and stops in that
  file. The old spelling refuses with exit 2 and names the new one.

**Twelve tools, and two instruments beside `measure-leaf`.**
