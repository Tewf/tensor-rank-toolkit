# Bringing an algorithm in

`sparsify-operator evidence/fixtures/plinopt/2x2x2_7_Winograd_L.sms` reads the published 7x4
integer operator, header `7 4 R`, 14 nonzeros including negatives. It reaches 10 nonzeros by every route the tool
offers, and none of them tripped the not-the-same-operator guard.

Comments travel both ways. Those matrices carry one `#` line before the header and
ours carry a two-line provenance block, and every tool on both sides skipped them
without a word.
