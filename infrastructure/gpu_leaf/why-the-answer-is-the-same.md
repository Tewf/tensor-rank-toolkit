# Why a kernel gives the leaf's own answer

A leaf hands its caller a rank-one basis, and the greedy that builds one is a
loop-carried dependency: whether a candidate is kept depends on what was kept
before it. A GPU has no order. So the argument that this is the same answer, and
the assertion that backs it up.

## The argument

The filter and the greedy are separable, and only the greedy has an order.

`Gf2SpanBasis::contains` is a pure function of the candidate and the span, and
**the span does not change while a scan runs**: `by_scanning_the_pool` builds
`reachable` once, before the loop, and only ever reads it. So which candidates
survive the membership test does not depend on the order the candidates are
tested in, and testing all of them at once is the same filter as testing them one
at a time.

`Gf2SpanBasis::try_add` is the part that carries state, and it runs only on
survivors, of which a leaf needs `k` out of four billion. It stays on the host.
Run over the survivors **sorted by index**, it visits exactly the indices the
sequential loop visits, in exactly the order the sequential loop visits them, and
so keeps exactly the same maps. That is why
[`gpu_leaf.h`](gpu_leaf.h) sorts before returning: the threads that wrote the
survivor buffer were not ordered, and the sort is what puts the order back.

The same holds for the walk, where the filter is `gf2_is_rank_one` rather than
`contains` and is a pure function of the subspace element alone.

**So filtering in parallel and running the greedy sequentially in index order is
bit-identical to the sequential loop.** The kernel also regroups the span rows by
which word their pivot is in, which changes nothing either: the rows arrive in
reduced row echelon form, so no row carries another's pivot bit, so reducing
against one never changes whether another applies.

## The assertion

An argument is not a test. `measure-leaf check` asks thirteen questions of both
machines, over four shapes and both routes, and for each one compares:

- the **survivor sets, element for element** by index, the kernel's against
  [`host_reference.h`](host_reference.h)'s. Equal sizes would not be evidence,
  so the sizes are never what is compared;
- the **rank-one basis**, entry by entry, the host greedy's over the kernel's
  survivors against what the shipped `Gf2Leaf` returned for the same question.

Two of the thirteen are built to have 65 535 survivors, because a comparison of
two empty lists compares nothing, and one has none, because an empty answer is
also an answer. All thirteen agree.
