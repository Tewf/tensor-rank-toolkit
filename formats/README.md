# Reading and writing the three file formats

Every input this repository runs on is a file, written out in full, so results
are checked against bytes rather than against a generator that could drift with
the code it feeds. Three formats, and the reason there are three.

| File | Holds | Read by | Written by |
|---|---|---|---|
| `.tensor` | a bilinear map, as one dense matrix per output coordinate | [`tensor_file.h`](tensor_file.h) | [`tensor_file.h`](tensor_file.h) |
| `.matrix` | one operator, entries integer or `4/9` | [`dense_matrix_file.h`](dense_matrix_file.h) | [`dense_matrix_file.h`](dense_matrix_file.h) |
| `.sms` | the same, in the sparse format LinBox, Givaro and PLinOpt speak | [`sms_file.h`](sms_file.h) | [`sms_file.h`](sms_file.h) |

The last two columns naming the same header is the point of the table rather
than a redundancy in it. A format whose halves sit in different layers cannot be
round tripped, so its reader is only ever checked against files somebody typed,
and the tensor format was in exactly that state while `write_tensor` lived
inside `make-tensor`. The commands write through these headers now:
`make-tensor` through `write_tensor`, `minimise-rank --emit-operators` through
`write_sms_file`.

Both text formats ignore blank lines and `#` comments, so a fixture can say what
it is. Both refuse what they do not understand: a parse error throws rather than
returning a half-built object that could be misread and lead to incorrect results.

## Why a matrix file is rational and a tensor file is not

A bilinear map lives over `GF(p)`, and the file names the `p` on its first line.
An operator does not: the entries of Strassen's alternative-basis operator are
ninths, and the quantity being minimised is how many of them are zero, so an
entry that rounds is an answer to a different question. Integers are rationals,
which is why one format carries both and why the rank search can hand its
recovered operators straight to the sparsification.

## SMS, and why it is here

`.sms` is what the surrounding ecosystem reads: LinBox and Givaro, the exact
linear algebra libraries this repository builds on, both speak it. This
implementation reads and writes it, enabling direct handoff to external solvers.
Reading is by extension, so
`sparsify-operator operator.sms` needs no flag.

Entries are one-based `row column value` triples after a `rows columns type`
header, and `0 0 0` terminates. The terminator's value is ignored, as the
format's own writers vary on it. Which letter the type is, and how much that is
known to matter, is [`sms_file.h`](sms_file.h).

It is worth exchanging files with somebody only if you have checked that you can.
Both directions have been run against PLinOpt's own binaries, and his checker
confirms the published 14 products on `f2_5x5` and 10 on `f3_3x6` from our
operators alone:
[`plinopt_interoperability/`](plinopt_interoperability/README.md).
