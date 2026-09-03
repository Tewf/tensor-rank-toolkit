# Where each side of the break-even comes from

Keys are [`../../references.md`](../../references.md). Write `s = dim span(T)`,
`L = target - s` for the levels of augmentation, `n = a + b` for the factored
degree, `k` for the size of the cell set a call is made about.

## The saving side has a theorem under it

**`[mckay1998]` Theorem 3, verified verbatim:** `N1 <= c*N2`, where `N1` counts the
times the test `Y in m(Y)` is *made*, `N2` the times it is **passed**, and `c` is
the average number of `Aut(X)`-orbits on `L(X)` over **reducible** objects only.
Roots are never tested. It is a statement about procedure `scan`, not `scan2`.

Two consequences, and both are load-bearing.

**The price is charged to the tree that is kept, never to the tree that is
avoided.** `augmentations()` already hands `is_canonical_augmentation` one
candidate per `Stab(X)`-orbit, so the tests made at a node *are* `|L(X)/Aut(X)|`
and McKay's `c` is exactly this route's branching factor. The whole comparison is
therefore per node, which is why nothing below ever multiplies a cost by the plain
tree's size.

**The saving is capped by the group.** The `G`-orbits on the `j`-subsets of the
pool number at least `C(|P|,j)/|G|`, so `rho <= |G|`, with equality approached only
where the action on what the search visits is free. Taking `rho = |G|` is
deliberately generous to the route being priced: a refusal then refuses it on its
own best case.

## The price side has two operations, and only one is bounded

**The canonical image is bounded, and at these shapes polynomially.**
`[linton2004]` §3.4 splits the work four ways:

| part | cost | charged to |
|---|---|---|
| stabiliser chain for `G` | `O(n log n (log\|G\|)^4)`, `[seress2003, Thm 4.5.5]` | the presentation, once |
| `k` base changes | `k * O(n (log n)^2 (log\|G\|)^2)` | each call |
| orbits under `G_{i-1}` | `O(k n (log n)^c)` in total | each call |
| each candidate | `O(k (log n)^c)` | each call |

Linton charges the first to nobody: "in most applications this will be known
anyway, since `G` will be the overall symmetry group of the problem, and so this
work need not be accounted to the smallest image algorithm". Here it is not known
in advance, so it is the entry fee `F` and is charged once per search.

**The candidate count is the only unbounded part**, and he gives three bounds
summed over iterations: `O(k!)`, `O(n^k)` or `O(2^n)` whichever is smaller, and
`k|G|`. **Here `k|G|` is smaller than either of the others by many orders**, and
his third conclusion applies word for word: "if `|G|` grows polynomially in `n`
then the running time is polynomial in `n` for any `k`". On the factored
presentation it does: `log|G| / log n` is 1.58, 2.00, 2.41, 1.90 and 2.22 at
`<2,2,2>`, `<2,2,3>`, `<2,2,4>`, `<2,3,3>` and `<3,3,3>`, so `|G| ~ n^2`
throughout. He adds that "experiment suggests that the actual number of candidates
is much smaller", which is why the constants in
[what-it-costs-here.md](what-it-costs-here.md) are measured and the bound is quoted
rather than used.

**The setwise stabiliser is not bounded.** It is GI-hard (`[luks1993]` Prop. 4.2,
with STAB, INTER and CENT polynomial-time equivalent in Prop. 4.3), not known to be
in P, quasipolynomial through Babai's string isomorphism result, and implemented
here by a backtrack search with no proven subexponential worst case. Every number
attached to it is measured and none of them is a bound. It is the one term a new
shape can falsify outright rather than merely shift.

## The formula

One canonical node, reading `descend` and `is_canonical_augmentation` line by line:
two pool scans of its own, one setwise stabiliser, and then per candidate child one
more pool scan and one canonical image per candidate parent plus one for the
distinguished cell.

    scan  = |P| * (membership test)
    node  = 2*scan + S + c * ( scan + (parents + 1) * C )
    pi    = node / (one plain node)
    rho   = |G|
    pays  <=>  pi/rho + F/(plain sweep)  <  1

`parents` is `(p^L - 1)/(p - 1)`, the hyperplanes of the quotient, of which
`candidate_parents` keeps only the reachable ones, so it is an upper bound, and
the one place this model is cautious rather than generous.

## And at `L = 1` there is no tree, so none of the above applies

Everything above compares two trees through a per-node ratio. At one level of
augmentation there is one internal node, the root, and its children are leaves,
so a per-node ratio divides two numbers that mean different things.

Worse, `rho` is not merely near 1 there, it **is** 1, and for a reason rather
than by coincidence. Both routes are handed the same generators and both take the
exact quotient: the baseline by `least_in_orbit`, which opens `i` exactly when
`i = min(O(i) ∩ [from, |P|))`, and this route by `orbit_representatives`, which
returns one index per orbit. At the root `from` is 0, so both emit exactly one
child per `G`-orbit of the pool, and the node count of either is `orbits + 1`.
Counted separately, that is 5, 5, 5, 10 and 13 orbits against node counts of
6, 6, 6, 11 and 14.

On the shipped `<2,2,2>` fixture, `price-canonical-route
fixtures/matmul_2x2x2.tensor -s matmul 2 2 2 --target 5` prints `orbits of the
pool: 5, sum of squared sizes 10773`, the exact row this section counts by hand.

So the one-level break-even is between two **roots**, written out:

    plain root       R * sum |O_i|^2                    R  one least_in_orbit step
    canonical root   scan + A*|P| + S + (r+1)*C         A  one orbit-pass step
    leaves           r * (canonical scans, plain walks p^target)
    both             building the pool, |P| elements

    pays  <=>  R * sum |O_i|^2  >  everything on the right

**The two roots differ in order.** `least_in_orbit` reaches an orbit breadth
first and asks `std::find` over a `seen` list that grows to the whole orbit, so
naming one representative costs `O(|O|^2)` and the root's sweep costs
`Theta(sum |O_i|^2)`; `orbit_representatives` marks each element once and costs
`Theta(|P|)`. Neither is a group operation and neither is `[mckay1998]`'s: this
clause is a statement about the **baseline's** orbit test, which is why getting
the row right is not the same as taking the route.

`sum |O_i|^2` has no closed form here and is the one input to the predicate that
costs anything: one `O(|P| * generators)` pass, 1% of the decision it prices.
Bounded above by `|P| * |G|` and below by `|P|^2 / r`, and the upper bound is
125x the truth at `<3,3,3>`, so the bound is not usable as an estimate. That is
the same mistake `rho <= |G|` made, and it is refused the same way: unmeasured,
the clause declines.
