; CLAIM:  the witness tensor T_ce = e0 (x) I (its first a-slice is the 2x2 identity,
;         its second a-slice is zero) has a rank-<=2 decomposition over GF(2)
; EXPECT: sat
; The companion to no_rref_decomposition_of_witness: this shows T_ce is genuinely
; rank-2 decomposable, so that the other claim's unsat means device 10's RREF
; constraint LOSES a real decomposition, not that none existed. Witness Z3 will
; find: a1 = a2 = (1,0), b1 = c1 = (1,0), b2 = c2 = (0,1).
(assert (and      (dec 0 0 0)  (not (dec 0 0 1)) (not (dec 0 1 0))      (dec 0 1 1)
             (not (dec 1 0 0)) (not (dec 1 0 1)) (not (dec 1 1 0)) (not (dec 1 1 1))))
(check-sat)
