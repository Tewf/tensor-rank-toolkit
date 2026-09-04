# The mistake, and the twelve pencils that caught it

What this module first shipped, why it was wrong, and the measurements that
settle where the classical formula holds. Which parts of this the literature had
already answered is [`what-the-literature-settles.md`](what-the-literature-settles.md);
[`README.md`](./) is what the module claims now.

It first shipped Ja'Ja's formula with the **GF(p)** elementary divisors and called
the result the rank. `test_pencil_agreement` refused it: on `(I_4, C)` over GF(2)
with `C` the companion of `x^4 + x + 1`, the formula said 5 and the exhaustive
search **proved** there is no algorithm with 5 products, walking 1 897 576 nodes
to the end of the tree, then exhibited one with 6.

The classical formula is a theorem over an algebraically closed field. Over a
small one it is only a lower bound, because the construction that attains it
interpolates and a small field does not carry the points it wants to evaluate
at. The published statement of that, for this tensor, is `[sumi2009, Thm. 3.3]`:
Ja'Ja's count holds under `Card(K) >= deg p_1(A)` and the theorem claims
nothing without it.

**None of that is this repository's discovery.** `[sumi2009, Prop. 3.4]` is the
published counterexample when the hypothesis fails: `(I_3, C)` with `C` the
companion of `x^3 + x + 1` over GF(2) has rank at least 5. That pencil is in the
table below, where exhaustion gives exactly 5 against a GF(q) count of 4.

**This page called it "Winograd's condition" with no key. The name turns out to
be fair; the word that was wrong is *same*.** `[lsw1983]` p. 287 item (ii) has
Winograd himself as an author and credits to his 1977 paper both the structure
of every optimal algorithm and the consequence drawn from it in the same breath:
a length-`2n-1` algorithm for the product of two polynomials of `n` coefficients
multiplies `(b_i·x(a_i))·(c_i·y(a_i))` at distinct `a_i`, so *"at least 2n − 2
distinct elements from F (namely, the a_i) are needed ... and if |F| < 2n − 2
then μ(r) ⩾ 2n"*. `[kaminskibshouty1989]` p. 150 puts it in the words this page
reached for on its own, citing the same paper: an optimal algorithm *"must
evaluate the multiplicands at a minimum of 2n distinct points, multiply the
samples, and interpolate the result. However, in finite fields, this method
fails if 2n exceeds the number of field elements."* So the mechanism, and
crediting the mechanism to Winograd, were both right.

**What is wrong is that it is not the same condition.** Winograd's threshold is
`2n-2`, and it is about multiplying two polynomials of `n` coefficients, or
that product modulo a `P` of degree `n`, which is `[lsw1983]` item (iii) and
needs his 1979 *On multiplication in algebraic extension fields* on top. A
pencil is multiplication by a polynomial of degree `≤ 1`: two slices, not `n`,
and its threshold is `deg p_1(A)`. The two are analogues across different
tensors, and the one this module needs is `[sumi2009]`'s.

`[winograd1977]` is **not read** and no numbered result of its own is quoted
here, because nothing reachable gives its internal numbering. It is named
through `[lsw1983]`, the way `[gabriel1972]` is named through `[brion2008]`.

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
