; ---------------------------------------------------------------------------
; The domain this repo makes checkable claims about, written ONCE and audited once.
; Every claim file is prepended with this, so a claim stays short enough to read.
; Fixing the vocabulary once, and auditing it once, is what keeps the claims honest.
; ---------------------------------------------------------------------------
; A rank-<=2 bilinear decomposition of a 2x2x2 tensor over GF(2), which is the
; smallest setting in which device 10's soundness can fail. GF(2) is the Booleans
; with + = xor and * = and. Each factor a_i, b_i, c_i lives in GF(2)^2, written as
; two Bools (index 0 and 1). A rank-one term is the outer product a_i (x) b_i (x) c_i;
; the decomposition is the xor of two such terms, entry by entry over the 2x2x2 grid.
(declare-const a1_0 Bool) (declare-const a1_1 Bool)
(declare-const a2_0 Bool) (declare-const a2_1 Bool)
(declare-const b1_0 Bool) (declare-const b1_1 Bool)
(declare-const b2_0 Bool) (declare-const b2_1 Bool)
(declare-const c1_0 Bool) (declare-const c1_1 Bool)
(declare-const c2_0 Bool) (declare-const c2_1 Bool)

; factor selectors: the value of a factor at coordinate p in {0,1}
(define-fun a1 ((p Int)) Bool (ite (= p 0) a1_0 a1_1))
(define-fun a2 ((p Int)) Bool (ite (= p 0) a2_0 a2_1))
(define-fun b1 ((q Int)) Bool (ite (= q 0) b1_0 b1_1))
(define-fun b2 ((q Int)) Bool (ite (= q 0) b2_0 b2_1))
(define-fun c1 ((r Int)) Bool (ite (= r 0) c1_0 c1_1))
(define-fun c2 ((r Int)) Bool (ite (= r 0) c2_0 c2_1))

; the decomposition's entry at (p,q,r): term1 xor term2, each term a triple AND
(define-fun dec ((p Int) (q Int) (r Int)) Bool
  (xor (and (a1 p) (b1 q) (c1 r))
       (and (a2 p) (b2 q) (c2 r))))

; device 10's constraint: the first-factor matrix A = [[a1_0,a1_1],[a2_0,a2_1]]
; is in reduced row echelon form. Over GF(2) the 2x2 RREF matrices are exactly
; these five (rank 0, the three rank-1 with the pivot row on top, and rank-2 = I).
(define-fun rref_first_factors () Bool
  (or (and (not a1_0) (not a1_1) (not a2_0) (not a2_1))   ; 0
      (and      a1_0  (not a1_1) (not a2_0) (not a2_1))   ; [[1,0],[0,0]]
      (and (not a1_0)      a1_1  (not a2_0) (not a2_1))   ; [[0,1],[0,0]]
      (and      a1_0       a1_1  (not a2_0) (not a2_1))   ; [[1,1],[0,0]]
      (and      a1_0  (not a1_1) (not a2_0)      a2_1)))  ; [[1,0],[0,1]] = I
