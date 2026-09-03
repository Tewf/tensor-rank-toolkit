# The predicate is sound, and it is still wired to nothing

`price_canonical_route` gets all ten swept rows and says the route pays at one of
them. This page is why that is not an instruction, and it is the answer to a fair
objection: a predicate measured as *right* about `<3,3,3>` and then consulted by
nobody looks like a result being ignored.

The verdict it feeds is one row of
[`../../how-the-search-works/what-to-wire.md`](../../how-the-search-works/what-to-wire.md).

Run directly on the shipped `<3,3,3>` fixture, `price-canonical-route
fixtures/matmul_3x3x3.tensor -s matmul 3 3 3 --target 10` prints `orbits of the
pool: 13, sum of squared sizes 9.93945e+09` and ends `verdict: canonical
augmentation pays here`, the one row this page is about.

## There is a condition, it is sound, and it is cheap to test

- **At `L >= 2` the route loses by 1.9x to 15x, structurally.** A plain node
  scans the live suffix and a canonical node must scan the whole pool, so the
  per-node price grows with the pool where the node saving does not:
  [the-suffix-and-the-whole-pool.md](the-suffix-and-the-whole-pool.md).
- **At `L == 1` the two routes visit the identical tree**, `orbits + 1` nodes
  either way, and the route wins at `<3,3,3>`: 2.26 s against 4.87 s.

Everything the clause needs is knowable before a node opens. `|P|`, the degree,
`|G|`, `L` and the generator count are closed form and free; `sum |O_i|^2` is one
`O(|P| * generators)` pass, 51 ms at `<3,3,3>` against the 4.9 s it prices.
Nothing in it is measurable only after the search, which is the test a predicate
has to pass to be one at all.

## And the win it finds is the baseline's, not `[mckay1998]`'s

Both roots name one child per orbit, from the same six generators, and get the
same children. The plain route asks `least_in_orbit` once per pool element: it
walks the orbit breadth first and asks `std::find` over a `seen` list that grows
to the whole orbit, so the sweep costs `Theta(sum |O_i|^2)`. The canonical route
asks `orbit_representatives`, which marks each element once: `Theta(|P|)`.

| shape | `\|P\|` | `sum \|O_i\|^2` | `least_in_orbit` | `orbit_representatives` | children |
|---|---|---|---|---|---|
| `<2,2,2>` | 225 | 1.08e4 | 29.2 us | 27.3 us | 5 |
| `<2,2,3>` | 945 | 2.42e5 | 286 us | 123 us | 5 |
| `<2,2,4>` | 3 825 | 5.32e6 | 4.19 ms | 502 us | 5 |
| `<2,3,3>` | 32 193 | 1.73e8 | 88.9 ms | 4.27 ms | 10 |
| `<3,3,3>` | 261 121 | 9.94e9 | **5.05 s** | **51.2 ms** | 13 |

**The plain route's entire 4.87 s run at `<3,3,3>` target 10 is that one call.**
So the predicate fires there because the competitor is slow, on the one level of
a sweep where it can fire at all, at a pool 13x past where `--route auto` has
already left the pool for the solver. A real sweep starts at
`rank_lower_bound`, 14 at `<3,3,3>`, so it never reaches `L = 1` at all. Wiring
it would put an artefact on a path nothing takes.

The test says the same thing in the only way a test can. Two sabotages sit beside
the pinned decision, and either takes `<3,3,3>` back off the route: withhold the
orbit statistics, or make the baseline's orbit test 100x cheaper. **A predicate
that survives neither is not describing canonical augmentation.**

## What to do instead, and it is 99x rather than 2.15x

At a node whose live suffix is the whole pool, `orbit_representatives` gives the
identical children for `Theta(|P|)`. It costs one 4-byte array over the pool
(1 MB at `<3,3,3>` against that pool's own 184 MB), which is exactly the memory
`least_in_orbit` gave up in order to reach `<4,4,4>`, where the same array is
17 GB. **The trade is real and it is conditional on the pool being
materialised**, which it is on every shape this route can run at.

`../../orbit_reduction/isomorph_rejection.cpp` says "an orbit inside a live
suffix is small and a linear scan of it beats hashing". That is true of a deep
node, where the orbits are 45 elements, and false of a root, where they are
49 392. Holding `seen` as a hash set instead is **not** the fix: measured 2.68 s
against 5.05 s, 1.9x, because what remains is the walk and not the membership
test.

## Nor is a cheaper partial break, and it is the same measurement

The partial rule already exists: `--orbit-test generators`, Crawford lex-leader
`[crawford1996]` restricted to one generator applied once. It is sound, it
refuses fewer duplicates, and it is the only kind available: `[anders2024,
Thm. 1.1]` puts graph isomorphism in co-NP if a polynomial-time **complete**
symmetry-breaking predicate for row-column symmetries exists, and the barrier
survives reordering, circuit encodings and added variables.

At the `<3,3,3>` root it costs **808 us** against 5.05 s and opens **10 732**
children against 13. That is 6 250x cheaper for 825x the branching, at a root
every child of which is a subtree, and it compounds:
[`../../orbit_reduction/what-partial-rejection-leaves.md`](../../orbit_reduction/what-partial-rejection-leaves.md)
measures 5.1x and 18.0x the nodes on two refutations.

**Cheapening the rejection is the wrong axis here.** `[katsirelos2010]`'s finding
is that a stronger break is not thereby a slower one, and this is that case at
its sharpest: the *exact* answer is available in `Theta(|P|)`, so a weaker rule
has nothing left to buy.

## Where the route does win outright is a question, not a shape

Counting solution subspaces up to the group, where the plain enumerator must
store one canonical code per distinct object and re-walk every orbit: 22 778x
fewer nodes and 11.8x the clock at `<2,2,2>` target 7, the margin growing with
the group ([`../deduplication-cost.md`](../deduplication-cost.md)). That is
already wired, in `enumerate-subspaces`. Deciding is not counting, and
`decide-rank` is deciding.
