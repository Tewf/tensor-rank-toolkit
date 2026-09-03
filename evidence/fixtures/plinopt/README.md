# PLinOpt's own operators, unmodified

Thirteen `.sms` files copied byte for byte from `data/` in **PLinOpt**, by
Jean-Guillaume Dumas, Bruno Grenet, Clément Pernet and Alexandre Sedoglavic
([github.com/jgdumas/plinopt](https://github.com/jgdumas/plinopt)), which is
distributed under the **CeCILL-B** free software licence. A copy of that licence
is [`Licence_CeCILL-B_V1-en.txt`](Licence_CeCILL-B_V1-en.txt), beside the files
it covers, and it is the licence they stay under: nothing in the MIT LICENSE at
the repository root applies to them. Its Article 5.3.1 is why the licence sits
here rather than being linked, and Articles 8 and 9 are the warranty and
liability disclaimers it asks a redistributor to carry.

They are here because interoperability that is only claimed is worth nothing.
The test suite reads *his* bytes rather than something written here to look like
them, so a change to the reader that breaks the exchange fails a test instead of
being discovered by him.

| File | What it carries that the tests need |
|---|---|
| `2x2x2_7_Strassen_{L,R,P}.sms` | a complete triple, so the map it computes can be rebuilt and compared with [`../matmul_2x2x2.tensor`](../matmul_2x2x2.tensor) |
| `1o1o2_3_Karatsuba_{L,R,P}.sms` | a second complete triple, over GF(2), typed `M`, rebuilding [`../f2_2x2.tensor`](../f2_2x2.tensor) |
| `3x4x7_63_rational_{L,R,P}.sms` | a shape whose two operands differ, L 63x12 against R 63x28, so that a slice built transposed is caught. Both other triples have L and R the same width |
| `2x2x2_7_DPS-smallrat-12.2034_{L,R,P}.sms` | rationals, triples not in lexicographic order, and a modulus the algorithm does not survive: ninths and halves, so GF(2), GF(3) and GF(5) are refused and GF(7) rebuilds the map |
| `2x2x2_7_Winograd_L.sms` | integers under an `R` header, and negatives |

Three families of his `data/` are deliberately **not** here, and each is a
finding rather than an omission: the `-X` operators, whose entries are
polynomials in an indeterminate; `4o4o4_F32_Montgomery_P.sms`, which holds four
matrices in one file; and the 32x32x32 operators, which are large and prove
nothing the small ones do not. What was measured across all 153 of them, and
what to install to repeat it, is
[`../../formats/interchange/`](../../../core/formats/interchange/README.md).
