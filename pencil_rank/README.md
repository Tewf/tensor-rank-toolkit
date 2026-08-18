# Rank of a two-slice tensor, without a search

A tensor with two slices is a **matrix pencil** `A + xB`, and Kronecker's theory
says everything about it that strict equivalence can: a pencil is a direct sum of
singular blocks, indexed by **minimal indices**, and a regular block, described by
its **elementary divisors**. Both are computed here by exact linear algebra over
GF(p), in polynomial time and with no candidate pool at all.

`decide-rank-by-pencil <tensor>` prints the form and what it implies.

## What is computed, and how strongly it is claimed

| | Claim |
|---|---|
| the Kronecker structure | **exact**, and checked three ways against itself |
| rank over the algebraic closure | **exact**, by `[jaja1979]`, so a **proved lower bound** over GF(p) |
| rank over GF(p) | **exact** when the pencil is diagonalisable there; a bound otherwise |

The structure is checked rather than trusted. The regular part's size is counted
along the rows and along the columns, and separately as the total degree of the
elementary divisors; `kronecker_structure` throws unless all three agree.

## The thing this module was wrong about, and the measurement that caught it

It first shipped Ja'Ja's formula with the **GF(p)** elementary divisors and called
the result the rank. `test_pencil_agreement` refused it: on `(I_4, C)` over GF(2)
with `C` the companion of `x^4 + x + 1`, the formula said 5 and the exhaustive
search **proved** there is no algorithm with 5 products, walking 1 897 576 nodes
to the end of the tree, then exhibited one with 6.

The classical formula is a theorem over an algebraically closed field. Over a
small one it is a lower bound, because the construction that attains it wants to
evaluate at more points than `P^1(GF(q))` has. That is Winograd's condition,
the same one that governs polynomial multiplication over small fields.

Twelve pencils, all settled by exhaustion, with both counts this module reports:

| pencil | field | closure | GF(q) count | rank |
|---|---|---|---|---|
| `(I_2, C)`, `x^2+x+1` | GF(2) | 2 | 3 | **3** |
| `(I_3, C)`, `x^3+x+1` | GF(2) | 3 | 4 | **5** |
| `(I_4, C)`, `x^4+x+1` | GF(2) | 4 | 5 | **6** |
| `(I_2, C)`, `x^2+1` | GF(3) | 2 | 3 | **3** |
| `(I_3, C)`, `x^3+2x+1` | GF(3) | 3 | 4 | **4** |
| `(I_2, C)`, `x^2+2` | GF(5) | 2 | 3 | **3** |
| `(I_2, N_2)` | GF(2) | 3 | 3 | **3** |
| `(I_3, N_3)` | GF(2) | 4 | 4 | **4** |
| `(I_4, N_4)` | GF(2) | 5 | 5 | **6** |
| `(I_3, N_3)` | GF(3) | 4 | 4 | **4** |
| `(I_4, N_2 + N_2)` | GF(2) | 6 | 6 | **6** |
| `(I_4, C_2 + N_2)` | GF(2) | 5 | 6 | **6** |

Read it two ways. The proved closure bound is sound and often loose, because an
irreducible splits over the closure and stops costing anything. The GF(q) count
is sharper on every row, equals the rank on nine of twelve, and exceeds it on
none, which is why it is reported; it is **not proved** to do either, which is
why it is labelled PROVISIONAL and not called an answer.

The three rows where both fall short share a shape: a block whose size is large
against the field. `d = 3` and `d = 4` over GF(2) fail, `d = 3` over GF(3)
does not, and `d = 2` never does.

## What would close it

A per-block cost `c(p^e, q)` proved rather than measured. The measurements are
consistent with `c` being 1 while the field has enough points for the block and
larger below that, which is a conjecture and is written here as one. The
exhaustive search settles any single case up to about 4x4 over GF(2), so a
proposed `c` can be refuted cheaply, which is the useful half.

## Files

`polynomial` is arithmetic over GF(p)[x]; `prime_power_factors` turns a
polynomial into the degrees and exponents of its prime powers, deterministically;
`pencil_divisors` diagonalises the pencil over GF(p)[x] twice, forwards for the
finite divisors and reversed for the infinite ones; `minimal_indices` reads the
singular structure off the ranks of one block system; `kronecker_structure` puts
them together and refuses to return anything the three counts disagree about.
