# What this interface will not say

`infrastructure/cli/exit_code.h` states the rule every tool here keeps: **a question that was
not answered is not a question answered no.** A budget running out proves
nothing, and folding it into a refusal turns giving up into a lower bound. That
mistake has been published from this repository once already, which is why half
its safety machinery exists.

A front end is exactly where it would be made a second time, because a screen
has room for one word and the honest word is three. So the vocabulary is kept
whole, in [`outcome.py`](outcome.py), and nothing anywhere shortens it.

## The six the toolkit speaks

| exit | word | what may be concluded |
|---|---|---|
| 0 | yes | an algorithm with that many products exists, and it was verified |
| 1 | no | proved that none exists |
| 2 | usage | the arguments did not parse, so no question was asked |
| 3 | undecided | a budget was exhausted. Nothing is proved, in either direction |
| 4 | unverified | an answer was produced and failed its own check |
| 5 | error | could not run at all |

A status that is none of these is reported as having no name, never as the
nearest one that does.

## The same rule in one word, so a screen can carry it

Words distinguish and colours do not, and a reader takes the colour first. Two
endings painted the same shade have been merged before the sentence under them
is read, so every card also carries `decides`: **found**, **proved**, or
**nothing proved**. It is `decide-rank`'s own vocabulary (that tool prints
`FOUND`, `NO` and `GAVE UP`) and it is one branch on the exit code, taken
before the tool's badge is applied and never from it. Only exit 1 reaches
`proved`. Everything that is not 0 or 1 lands in `nothing proved`: a budget, a
stop, the wall clock, a crash, and a status with no name. There is no path in
[`outcome.py`](outcome.py) by which giving up acquires a colour that says
otherwise, and the three are asserted from real runs below.

## The three the interface can cause, named as its own

**You pressed stop**, **the wall clock ran out**, and **the console shut down**.
All three are undecided in substance and none is the toolkit's answer, so each
card names which of them ended the run and says the rest in as many words. The
wall clock is a backstop and not a budget: it exists so a page cannot be left
waiting on a search that will not finish, and it decides nothing. Where a tool
has a budget of its own, that flag is filled in with the same number and appears
in the command, because a limit named in the line a reader retypes is evidence
and a limit imposed from outside is not.

## Why the badge and the sentence are two things

The tools do not answer the same question with the same code. Exit 0 from
`decide-rank` is a verified decomposition; from `minimise-rank` it is a descent
that finished, which claims an upper bound and nothing about the rank; from
`make-tensor` it is a file that got written. So each card leads with the tool's
own reading of its code, held once in `catalogue.py`.

The header's sentence for 0 and 1 then goes underneath, but only where it is the
faithful reading. Quoting "an algorithm with that many products exists, and it
was verified" under `make-tensor` would claim a verification that never
happened, which is the same fault as the one above with the sign reversed. For
2, 3, 4 and 5 it is always shown, because those four mean the same thing from
every one of them and are the ones that must never be softened.

A flag can change the claim too. `decide-rank-by-sat --emit-cnf` writes the
question and stops, so its exit 0 is a file that got written and not a
decomposition that was found, and `curve-bounds --table` prints a transcription
and minimises nothing. Both carry their replacement reading beside themselves in
`catalogue.py`, and both are asserted below.

## What is asserted, and against what

[`tests/check_web_interface.py`](tests/check_web_interface.py) drives the
interface over HTTP and reaches each of exit 0, 1, 3 and 5 from a real run of a
real binary, then asserts that the exit 3 case is undecided and is not reported
as a refutation, and that the four land on found, proved, nothing proved and
nothing proved. Asserting that against a table of codes would pass whatever the
interface did with them, so it is asserted against
`decide-rank fixtures/f2_5x5.tensor --target 13 --node-limit 1000`, which really
does run out of nodes.

Run against this build, that line ends:

```
build/methods/bilinear_rank/exhaustive_search/decide-rank fixtures/f2_5x5.tensor --target 13 --node-limit 1000
exit 3, GAVE UP: the node limit was reached, so nothing is decided. 1000 nodes.
```

The word is GAVE UP and the exit is 3, and `no` appears nowhere in that sentence.
