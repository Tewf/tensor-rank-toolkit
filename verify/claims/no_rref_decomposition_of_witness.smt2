; CLAIM:  no rank-<=2 decomposition of the witness tensor T_ce has its first-factor
;         matrix in reduced row echelon form
; EXPECT: unsat
; This is the device-10 soundness counterexample. T_ce = e0 (x) I is rank-2
; (witness_tensor_is_rank_two, sat), but every rank-2 decomposition forces both
; first factors to (1,0) - because T_ce's a-support is one-dimensional - so the
; first-factor matrix is [[1,0],[1,0]], which is rank 1 and cannot be RREF without
; a zero row, i.e. without dropping a term. Constraining the first factors to RREF
; therefore turns a satisfiable rank question UNSAT: a false lower bound. So the
; r^2 quotient does NOT transfer to a static SAT constraint as first-factor RREF,
; which is exactly the proof obligation the encoding-knowledge review left owed.
(assert (and      (dec 0 0 0)  (not (dec 0 0 1)) (not (dec 0 1 0))      (dec 0 1 1)
             (not (dec 1 0 0)) (not (dec 1 0 1)) (not (dec 1 1 0)) (not (dec 1 1 1))))
(assert rref_first_factors)
(check-sat)
