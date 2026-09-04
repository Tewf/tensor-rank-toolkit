# The whole algorithm, with all five pieces drawn in

[`README.md`](./) is the search **as it runs**, two of five pieces wired.
This is the same search with the other three drawn in and one device rule over
the whole of it, so a proposal can be read against what ships.
`[flag]` is a route that exists and is off by default, `[gap]` one that nothing
calls, and unmarked is what runs today. The verdict on each of the five, and why
three of them are no: [`what-to-wire.md`](what-to-wire.md).

## The setup, before a node is opened

```
decide(T, k):
    floor <- rank_lower_bound(T)          flattening, rank sums, Griesmer
    if k < floor: return NO               proved, with no node opened

    |P| <- (p^n - 1)(p^m - 1)/(p - 1)^2   the rank-one maps of the shape
    P   <- materialised  if |P| * (56 + 8nm) <= max_memory, else addressed
                                          the odometer: P[i] is arithmetic,
                                          lefts[i / R] (x) rights[i mod R]
    G <- requested_group(T)               generators, never elements;
                                          empty unless -s was given
    require G stabilises span(T), and P closed under G

    binary <- Gf2Leaf(P) if p = 2 else none        the packed leaf, both routes
    card   <- gpu_backend_available and p = 2
              and (n,m) in {4x4, 5x5, 9x9, 16x16}                        [gap]

    return descend(span(T), from = 0)
```

## The tree

```
descend(V, from):
    if not budget.try_consume_node(): return UNDECIDED
    if dim V > k:  return NO               no room left
    if dim V == k: return leaf(V)          the whole question, at depth
    for i in [from, |P|):                  a contiguous suffix, not a set
        if G and not least_in_orbit(G, i, from): continue    orbit reduction
        W <- V + P[i]
        if dim W == dim V: continue        P[i] is already inside
        if descend(W, i): return YES       or i+1, see below
    return NO
```

**`i` or `i + 1` is not a choice about reuse**, which this said until
2026-08-20. The quotiented search passes `i` and the plain one `i + 1`, to the
same tree: a map already inside `V` cannot raise the dimension, so the child
drops it either way.

**McKay replaces those two quotient lines rather than joining them** `[flag]`,
and drops the suffix with them: it quotients by the stabiliser of *this* node
rather than the root's group, and rules on the child rather than the candidate.

```
descend_canonical(V, from):
    S <- stabiliser_generators(the pool elements inside V)   rebuilt per node
    for i in one representative per orbit of S on {i : P[i] not in V}:
        W <- V + P[i]                      the whole pool, not a suffix
        if canonical_parent(W) is not the class of V: continue
        if descend_canonical(W, i + 1): return YES
    return NO
```

## The leaf, and the only place a device is chosen

```
leaf(V):                                   has V a rank-one basis?
    walk  <- p^(dim V), counted only up to |P|
    route <- Walk if 0 < walk < |P| else Scan       measured, not assumed
    work  <- walk if route = Walk else |P|
    device <- Cpu  if work < launch_floor          a launch costs more
              else first available of ranked_devices()

    if device = Gpu:                                                     [gap]
        survivors <- kernel(route, LeafQuestion(V, P), the whole range)
        if survivors.overflowed: retry in chunks, else fall back to Cpu
        return greedy over sorted survivors reaches dim V

    WALK on the host:   combination ^= rows[ctz(index)]   one xor, Gray order
                        is_rank_one(combination)          two rows, not a rank
    SCAN on the host:   residual ^= reduce(left (x) e_j)  one xor; dim leaves
                        residual = 0 => inside V          the inner loop
                        survivors sorted per left, then the greedy
```

**Both host routes carry rather than rebuild**, since 2026-08-20. Each returns
the same maps in the same order, and the CUDA kernel, written against the old
scan, agrees with the new one survivor by survivor on all thirteen shapes.

**The host leaf stops at `dim V` maps and the kernel does not**, so the card's
measured factors are for a leaf run to its end: every leaf of a refutation, and
no leaf of a witness.

Where `SortedSpan` would attach, and what each piece was measured at:
[`what-to-wire.md`](what-to-wire.md).
