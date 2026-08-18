# The claim this survey got wrong, and the search that would have caught it

**A correction about how this survey was written.** Its first version said
`[yang2025]` was "not implemented here", which was true of this repository and
read as though no implementation existed. One does, it is public, and I had
read the paper's abstract before writing that sentence without looking for its
code. The search that would have found it is `gh search repos "tensor CPD"`,
where it is the first hit; the searches I actually ran were for "tensor rank
SAT" and "matrix multiplication SAT solver", which return nothing at all. The
lesson: search the problem's own vocabulary, not the vocabulary of the method
you already chose.
