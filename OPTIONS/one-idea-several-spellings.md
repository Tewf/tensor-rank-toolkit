# One idea, several spellings

Thirteen tools share one argument grammar and not one vocabulary. Nothing below is
a bug and nothing below is changed by writing it down: a flag that has been
published under a name keeps it, and a reader who knows what a name means on one
tool should be told where it means something else. That is what this page is.

## The same idea, different names

| The idea | Where | Spelled |
|---|---|---|
| which machine answers | `decide-rank` | `--leaf-route auto\|scan\|walk` |
| | `factor-over-canonical-basis` | `--route auto\|exhaustive\|sat\|canonical` |
| | `curve-bounds` | `--route built-in\|chain\|enumeration` |
| | `decide-rank-by-sat` | `--backend cnf\|smt` |
| | `decide-rank-by-deflation` | `--refuter solver\|tree` |
| a seconds budget | `decide-rank-by-sat` | `--timeout N` |
| | `decide-rank-by-deflation` | `--candidate-timeout N` (same tunable) |
| | `curve-bounds` | `--solver-timeout N` |
| where the search starts | `decide-rank-by-sat` | `--from a` |
| | `factor-over-canonical-basis` | `--floor k` |
| | `decide-rank` | `--anchor map\|heuristic` |
| more than one core | five tools | `--threads N`, `0` for every core |
| | `decide-rank-by-deflation` | `--parallel`, which is all cores or none |
| write a file | `minimise-rank`, `tighten-rank-bound`, `decide-rank` | `--emit-operators <stem>`, three files from it |
| | `decide-rank-by-sat` | `--emit-cnf <path>`, the path itself |

**There is no `--seed` anywhere.** `walk-scheme --seeds N` is a count of
independent walks, not a seed value; each walk's seed is its index.

## The same name, different ideas

| Spelled | On | Means |
|---|---|---|
| `--steps N` | `minimise-rank` | which of the three descent stages to run, 1 to 3 |
| | `walk-scheme` | flips per seed, an accepted older spelling of `--flips` |
| `--from k` | `decide-rank-by-sat` | the bottom of the sweep |
| | `walk-scheme` | the k-product scheme to start walking from |
| | `tighten-rank-bound` | `basis` or `descent`: which root, an enum, not a count |
| `--node-limit N` | `decide-rank`, `decide-rank-by-deflation`, `factor-over-canonical-basis` | search-tree nodes, from `search_node_limit` |
| | `curve-bounds` | branch-and-bound nodes, from `ilp_node_limit`, whose default is 25x smaller |
| | `tighten-rank-bound` spells the same idea `--nodes N`, from no tunable at all |
| `--max-memory` | the three searches | the bulk-allocation budget (`set_memory_budget`), in bytes |
| | `decide-rank-by-sat`, `decide-rank-by-deflation` | the child solver's address-space cap via `RLIMIT_AS` (**not** the bulk-allocation budget), in bytes, from `sat_memory_megabytes`, see [`asking-a-sat-solver.md`](asking-a-sat-solver.md) |
| `--route` | see the table above | two different sets of values |

## What a bad value gets you

`infrastructure/cli/arguments.h` exists so that a refusal names the flag and quotes the word:
`--target abc` reported as `stoull` names neither, and there are five numeric
flags it could have been. Its typed parsers keep that promise. **It has no helper
for an enum-like value**, and the ten flags that take one hand-roll the branch
four different ways. Checked by running each against a fixture, 2026-08-21,
because two rows of this table had gone stale in the direction that flatters:

| Behaviour | Flags |
|---|---|
| names the flag and quotes the word | `--anchor`, `--leaf-route`, `--orbit-test`, `--device`, `--route` on `factor-over-canonical-basis` |
| reprints the whole usage; the word is never named | `--tune`, `--refuter`, `--route` on `curve-bounds` |
| reports a bad **value** as an unrecognised **flag** | `--from` on `tighten-rank-bound` |
| accepts anything as the non-default, silently | `--backend` |

So `decide-rank --orbit-test bogus` says `--orbit-test expects full or
generators, not 'bogus'`, while `tighten-rank-bound --from sideways` says
`unrecognised option: --from`, of a flag it recognises perfectly well, naming
neither the wrong word nor the two right ones. And `--backend smtt` runs the CNF
backend without a word, which is the quieter fault: it answers, and about a
question the caller did not ask.

**`--leaf-route` and `--anchor` were this page's two examples**, and both were
fixed in `decide_rank_main.cpp` (beside the comment naming the first as the
fault it declined to copy) without the page being sent back to check.
`--device`, which is right, never reached it at all. What is left is `--from`,
`--backend` and the three that reprint a usage block, and the fix for all five is
one helper in `infrastructure/cli/arguments.h` rather than five more hand-rolled branches. The
console refuses all ten from `catalogue.py`'s `values` before starting anything,
so this is a terminal wart and not a browser one.

Two more shapes worth knowing: `curve-bounds --table` and `--solvers` leave as
**exit 0** where every other early-out flag leaves as 2, and a malformed
`--points` term leaves as **exit 5** rather than 2, because its parser throws
outside the argument system.

## Spellings retired with the 2026-09-03 renaming

`deflate-strictly` is **`decide-rank-by-deflation`**, joining the family
whose members say the method after the question, and `lower-the-bound` is
**`tighten-rank-bound`**, because what it lowers is an upper bound and the
old spelling read as lower-bound work. As with `list-solvers`, the old
names' last job is this paragraph.
