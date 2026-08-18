# GF(p), SMT

```
(assert (= (ff.add (ff.mul (ff.mul a_l_i b_l_j) c_l_k) …) (as ff<t> F)))
```

`n₁n₂n₃` assertions over `r(n₁+n₂+n₃)` constants, and nothing else. The cost is
not in the file; it is in the Gröbner-basis procedure behind `QF_FF`
(`[ozdemir2023]`).
