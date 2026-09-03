# The tensors the literature argues about, run through both searches

The fixtures here are polynomial multiplication, because that is what the
polynomial fixtures cover. The code is not: a bilinear map `F^n × F^m → F^d` is an
order-3 tensor, so every tensor the complexity literature cares about fits the
same file format and the same two searches. This is what happened when they were
fed the famous ones, over GF(2), on one core of an i5-12450H.

Build them with `make-tensor --matmul p n m k` and `--cyclic p n`, `p` being the
field characteristic (2 throughout this table).

## What each search did

| Tensor | Naive | Strict descent | Plateau walk | Exact says | Known |
|---|---|---|---|---|---|
| `⟨2,2,2⟩` matrix multiplication | 8 | **8**, no first step | **7** · 0.11 s | **exactly 7** | 7 |
| `⟨2,2,3⟩` | 12 | **12**, no first step | not measured | **≥ 9** | 11 |
| `⟨2,3,3⟩` | 18 | **18**, no first step | not measured | nothing reachable | 15 |
| `⟨3,3,3⟩` | 27 | **27**, no first step | **24** · 38.1 s | out of reach | open, 19–23 |
| W state | 3 | 3 | | **exactly 3** | 3, border rank 2 |
| Cyclic convolution, length 5 | 25 | **10** | | **≥ 9** | |
| GF(16) over GF(2) | 16 | **9** | 9 · 0.69 s | **exactly 9** | ≥ 8 by de Groote |

"No first step" is the step 3 shortlist coming back empty, and it stops
`minimise-rank` and nothing else. The plateau column is the same heuristic
strand allowed to cross equal-cost maps, and on both matrix multiplication
shapes it was run on it improves on the descent. Its three timings are
`walk-scheme`, measured in
[`../oracle_guided_search/measurements/README.md`](../oracle_guided_search/measurements/README.md);
`⟨3,3,3⟩`'s 24 is `--flips 20000 --seeds 8`.

## Then one file per reading of that table

| | |
|---|---|
| [`decided-exactly.md`](decided-exactly.md) | the two tensors this repository settles outright, `⟨2,2,2⟩` and GF(16) |
| [`where-the-descent-stops.md`](where-the-descent-stops.md) | why no single rank-one map improves a matrix multiplication tensor, and what that does and does not bound |
| [`where-the-exact-search-stops.md`](where-the-exact-search-stops.md) | the price of every run made and of the ones refused, node by node |

For the state of the art on exactly these families, including matrix
multiplication over F₂ and cyclic convolution over F₂ and F₃, see `[wang2026]`,
*Automated Lower Bounds for Bilinear Complexity over Finite Fields*, which
raised the F₂ lower bound for `⟨3,3,3⟩` from 19 to 20 using orbit classification
under a symmetry group. That is the direction this search would have to go to
reach the sizes it currently cannot.
