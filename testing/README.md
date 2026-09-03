# testing/

The one assertion helper every module's tests use, so a failed check prints
what was expected and what arrived in the same shape everywhere. It holds no
tests of its own; each module keeps its tests beside its code, in its own
`tests/`.

Why it sits at the root instead of inside a module: the top of
[`CMakeLists.txt`](CMakeLists.txt).
