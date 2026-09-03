# What is checked automatically

Three tests, over the thirteen of PLinOpt's files vendored in
[`evidence/fixtures/plinopt/`](../../../evidence/fixtures/plinopt/README.md), so nothing here
reaches outside the repository for its inputs.

| | |
|---|---|
| [`../tests/test_sms_interoperability.cpp`](../tests/test_sms_interoperability.cpp) | that PLinOpt's bytes read, that what we write is what LinBox writes, and the four places the two readers were compared line by line |
| [`methods/bilinear_rank/greedy_heuristic/tests/test_operators_to_tensor.cpp`](../../../methods/bilinear_rank/greedy_heuristic/tests/test_operators_to_tensor.cpp) | that ⟨L,R,P⟩ means the same thing on both sides: three of PLinOpt's published triples rebuild three maps constructed here from their definitions |
| [`methods/bilinear_rank/greedy_heuristic/tests/check_operators_to_tensor.sh`](../../../methods/bilinear_rank/greedy_heuristic/tests/check_operators_to_tensor.sh) | the same through the command, which is the only place the order of three positional filenames is visible |

The runs above are not in the suite: they need PLinOpt's binaries, which are not
a dependency of this build. Neither is the sweep over all 153 of its matrices in
[`where-the-conventions-differ.md`](where-the-conventions-differ.md), which needs its `data/` as
well; the vendored thirteen are the subset that carries each thing the readers
disagreed about.
