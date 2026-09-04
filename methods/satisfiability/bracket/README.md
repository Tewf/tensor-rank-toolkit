# Locating the rank between the bounds

Two bounds come free: the flattening rank
([`tensor_flattening.h`](../../../core/linear_algebra/tensor_flattening.h))
below, the smallest product of two of the three dimensions above. Finding the
rank between them means asking a solver a sequence of decision questions, and
there is more than one order to ask them in. Encodings are
[`method/`](../method/); costs against the exhaustive search are
[`measurements.md`](../measurements.md).

## The floor no strategy can go below

Establishing `rank = r` needs a **yes at r** and a **no at r-1**, since one
refusal at `r-1` refutes everything under it. No schedule skips either, so every
strategy pays `cost(no at r-1) + cost(yes at r)` and competes only for the rest.
**On GF(16) that floor is 108.5 s of a 110 to 114 s search: the whole choice of
schedule is worth about three percent.** That number bounds every future idea in
this direction as well as the five below.

| | |
|---|---|
| [`every-question-priced.md`](every-question-priced.md) | what each `k` costs on GF(16), which prices every schedule at once |
| [`the-five-schedules.md`](the-five-schedules.md) | the five orders summed from that table, and which of them wins where |
| [`what-decides-it.md`](what-decides-it.md) | why the upper bound and not the order of questions is the lever |
| [`handing-over-the-bracket.md`](handing-over-the-bracket.md) | the sweep's bracket handed to `find_rank`, priced on all seven, and why it loses |
| [`two-corrections.md`](two-corrections.md) | the two schedule winners published here before, both wrong |
