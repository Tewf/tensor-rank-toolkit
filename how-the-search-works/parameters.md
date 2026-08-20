# Every parameter, and what it decides

The signatures the [pseudocode](README.md) stands for, with the meaning of each
argument. Names are the code's.

## `expand_subspace(field, subspace, pool, from, target, budget, products)`

| parameter | what it is | what it decides |
|---|---|---|
| `field` | the `ModularField`, `GF(p)` | all arithmetic, and whether the packed GF(2) leaf applies at all |
| `subspace` | the slices of `T` | the node the search starts from; every algorithm for `T` contains `span(T)` |
| `pool` | `vector<Matrix>` or `RankOnePool` | materialised or addressed. **The type is the odometer**: `RankOnePool::at(i)` derives element `i` from `i`, so nothing is stored |
| `from` | first candidate index | the suffix `[from, \|P\|)` this node may still use. Recursion passes `i`, not `i+1`, because one rank-one map may appear twice in a decomposition |
| `target` | `k` | the question. `dim V > k` prunes, `dim V == k` is the leaf |
| `budget` | `SearchBudget&` | `node_limit` and `leaf_element_limit`. Reaching either is exit 3 and decides nothing, which is the whole point of it being separate from a refutation |
| `products` | out | the decomposition, when the answer is yes |

## `expand_subspace_up_to_symmetry(..., group, ..., spread_over_cores)`

Adds two.

| parameter | what it is | what it decides |
|---|---|---|
| `group` | `vector<Automorphism>`, **generators** | the quotient. Empty means no quotient, and the search is the one above. Generators, never the enumerated group: `least_in_orbit` walks an orbit, and an orbit needs generators |
| `spread_over_cores` | bool | whether the top level is split across workers. A refutation's node count does not depend on it; a satisfiable search's does, measured in [`../exhaustive_search/what-threads-change.md`](../exhaustive_search/what-threads-change.md) |

## `rank_one_basis_of(field, span, pool, needed, scratch, budget, binary)`

The leaf.

| parameter | what it is | what it decides |
|---|---|---|
| `span` | `ReducedBasis` of `V` | the subspace being asked about |
| `needed` | how many independent rank-one maps to find | fewer found than `needed` means no rank-one basis, so the node fails |
| `scratch` | reused buffer | nothing but allocation; it exists so a leaf does not allocate |
| `binary` | `Gf2Leaf*` or `nullptr` | the packed GF(2) implementation of **both** routes. `nullptr` is the general Givaro path, which every other field takes |
| *(route)* | `set_leaf_route`, process-wide | `Auto` compares `p^dim` with `\|P\|`. `Scan` and `Walk` force one, for timing them on a single question |

## `SortedSpan(field, slices)`

| member | what it gives |
|---|---|
| `cost()` | `Σ r · (dim R_r − dim R_{r−1})`, the minimum-weight basis cost as a closed form |
| `has_rank_one_basis()` | `dim R[1] == dim V`, free **once the filtration is built**, and building it is a walk |
| `reached_by_rank(r)` | `dim R_r`, the filtration itself |

## The command-line surface

| flag | default | what it moves |
|---|---|---|
| `--target k` | none, sweeps | the question |
| `--anchor map\|heuristic` | `map` | the `subspace` the search starts from. From the map the answer is the true minimum; from the heuristic's own result it is the minimum only among algorithms containing that subspace |
| `-s, --symmetry` | `none` | the group above. `matmul n m k` is the closed form and needs no group built |
| `--orbit-test full\|generators` | `full` | which rule `least_in_orbit` applies at a quotiented node: the exact one, or the cheap partial one that tests the generator images only. Read only when `-s` is given, same verdict either way, and what the duplication costs is counted in [`../orbit_reduction/what-partial-rejection-leaves.md`](../orbit_reduction/what-partial-rejection-leaves.md) |
| `--leaf-route` | `auto` | the leaf route above |
| `--general-leaf` | off | forces the general field path, so the packed leaf can be timed against it |
| `--node-limit` | 5 000 000 | nodes before exit 3 |
| `--leaf-limit` | 100 000 000 | elements inside **one** leaf before exit 3, which no node limit can bound |
| `--threads` | 1 | `spread_over_cores` |
| `--max-memory` | 2 GiB | whether the pool is materialised or addressed |

Every flag of every tool, each with the measurement that chose its default:
[`../OPTIONS.md`](../OPTIONS.md).
