# Four ways to produce a failure that is not about SMS

Each of these looks like a defect in this layer. None is.

**Match the checker to the map.** `PMchecker` checks polynomial multiplication,
which is what everything in [`../../fixtures/`](../../fixtures/) is. `MMchecker`
is the
matrix-multiplication one and is a separate binary. Handing a `matmul_*` fixture
to `PMchecker` fails exactly as a bad operator would.

**Give `-q` a modulus.** With the flag absent or its value empty, `PMchecker`
falls back to rational arithmetic and reports
`****** ERROR, not a 1o2o3 MM algorithm******`, sometimes after dumping core. A
correct `GF(2)` algorithm fails this way, and nothing in the message says the
modulus is missing.

**Do not copy `-E -N` from his Makefile.** The `slpcheck` recipe passes exhaustive
common-subexpression elimination and exhaustive nullspace permutations, which
stalls past 300 s on a 9x14. Without them the round trip above finishes in
moments. Roughly 10x10 is where they stop being usable.

**Name the file `.sms`.** `sparsify-operator` picks its reader by that literal
four-character suffix, so any other name silently gets the dense reader. That is
ours to fix and is part of the deferred CLI work.

`optimizer` with no arguments aborts with a `LinBox::MatrixStreamError` rather
than printing usage. Ask it for `-h`.
