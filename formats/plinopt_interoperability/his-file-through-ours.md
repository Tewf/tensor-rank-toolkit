# His file through ours

`sparsify-operator fixtures/plinopt/2x2x2_7_Winograd_L.sms` reads his shipped 7x4
integer operator, header `7 4 R`, 14 nonzeros including negatives. It reaches 10
nonzeros by the row-basis heuristic and 10 by each exact oracle, and none of the
three tripped the not-the-same-operator guard.

Comments travel both ways. His matrices carry one `#` line before the header and
ours carry a two-line provenance block, and every tool on both sides skipped them
without a word.
