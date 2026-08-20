# One idea, several spellings

Twelve tools share one argument grammar and not one vocabulary. Nothing below is
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
| | `deflate-strictly` | `--refuter solver\|tree` |
| a seconds budget | `decide-rank-by-sat` | `--timeout N` |
| | `deflate-strictly` | `--candidate-timeout N` (same tunable) |
| | `curve-bounds` | `--solver-timeout N` |
| where the search starts | `decide-rank-by-sat` | `--from a` |
| | `factor-over-canonical-basis` | `--floor k` |
| more than one core | five tools | `--threads N`, `0` for every core |
| | `deflate-strictly` | `--parallel`, which is all cores or none |
| write a file | `minimise-rank` | `--emit-operators <stem>`, three files from it |
| | `decide-rank-by-sat` | `--emit-cnf <path>`, the path itself |

**There is no `--seed` anywhere.** `walk-scheme --seeds N` is a count of
independent walks, not a seed value; each walk's seed is its index.

## The same name, different ideas

| Spelled | On | Means |
|---|---|---|
| `--steps N` | `minimise-rank` | which of the three descent stages to run, 1 to 3 |
| | `walk-scheme` | flips per seed — an accepted older spelling of `--flips` |
| `--from k` | `decide-rank-by-sat` | the bottom of the sweep |
| | `walk-scheme` | the k-product scheme to start walking from |
| `--node-limit N` | `decide-rank`, `deflate-strictly`, `factor-over-canonical-basis` | search-tree nodes, from `search_node_limit` |
| | `curve-bounds` | branch-and-bound nodes, from `ilp_node_limit`, whose default is 25x smaller |
| `--max-memory` | the three searches | the pool budget, in bytes |
| | `decide-rank-by-sat`, `deflate-strictly` | the solver's cap, in megabytes, from `sat_memory_megabytes` |
| `--route` | see the table above | two different sets of values |

## What a bad value gets you

`cli/arguments.h` exists so that a refusal names the flag and quotes the word:
`--target abc` reported as `stoull` names neither, and there are five numeric
flags it could have been. Its typed parsers keep that promise. **It has no helper
for an enum-like value**, and the nine flags that take one hand-roll the branch
five different ways:

| Behaviour | Flags |
|---|---|
| names the flag and quotes the word | `--orbit-test`, `--route` on `factor-over-canonical-basis` |
| reprints the whole usage; the word is never named | `--tune`, `--refuter`, `--route` on `curve-bounds` |
| reports a bad **value** as an unrecognised **flag** | `--leaf-route` |
| accepts anything as the non-default, silently | `--anchor`, `--backend` |

So `decide-rank --orbit-test bogus` says `--orbit-test expects full or
generators, not 'bogus'`, and `decide-rank --leaf-route bogus`, four lines away
in the same file, says `unrecognised option: --leaf-route` — naming neither the
word that was wrong nor the three that would have been right. And
`--anchor heruistic` runs from the map without a word. The source comment at
`exhaustive_search/commands/decide_rank_main.cpp` already names the first of
these as the fault it declined to copy.

Two more shapes worth knowing: `curve-bounds --table` leaves as **exit 0** where
every other early-out flag leaves as 2, and a malformed `--points` term leaves as
**exit 5** rather than 2, because its parser throws outside the argument system.
