"""Seven starters that fill the console in, for somebody who has read none of this.

The interface's first screen used to be an empty box and a list of tool names,
which asks a newcomer to know both a map and a question before anything happens
at all. Each example below is a fixture, a tool and its flags, so the first thing
a reader does is press one button and read a real answer.

They are chosen to teach the distinction the toolkit is about rather than to show
the interface off: **found**, **proved no** and **gave up** are three of the
seven, on the same map, one flag apart. Nothing else here makes that difference
visible in twenty seconds.

Two rules they are held to, and `tests/check_web_interface.py` asserts both.
Every one runs on a build with no solver on `PATH`, because a starter that needs
an optional dependency teaches a reader that the toolkit is broken. And every one
ends with the badge named beside it, so an example that stops being true fails a
check rather than misleading the next reader.

`expect` is the badge such a run ends with, which is the tool's own word for its
exit code and not the interface's. It is never shown; it is what the check
compares against.

The last names a `flow` where the others name a `tool`, which is one of
`flows.py` and runs more than one command. Its `expect` is that flow's own word
and not a badge: `complete` says every step ended where its tool says it ends,
and what each step decided is on that step's own card.
"""

EXAMPLES = [
    {
        "title": "Strassen's seven exist",
        "why": "2x2 matrix multiplication in 7 products, found and then checked "
               "against the map it claims to compute.",
        "tool": "decide-rank",
        "fixture": "matmul_2x2x2.tensor",
        "options": {"--target": "7"},
        "expect": "yes",
    },
    {
        "title": "…and six are impossible",
        "why": "The same map, one product fewer. This is a refutation that ran to "
               "exhaustion, so it is a proof and not a failure to find one.",
        "tool": "decide-rank",
        "fixture": "matmul_2x2x2.tensor",
        "options": {"--target": "6"},
        "expect": "no",
    },
    {
        "title": "The same proof, quotiented by symmetry",
        "why": "One member per orbit instead of every candidate: 25 399 nodes "
               "become 648, for the same verdict. Watch the plan line.",
        "tool": "decide-rank",
        "fixture": "matmul_2x2x2.tensor",
        "options": {"--target": "6",
                    "-s": {"mode": "matmul", "n": "2", "m": "2", "k": "2"}},
        "expect": "no",
    },
    {
        "title": "A budget that runs out, which proves nothing",
        "why": "The third verdict, and the one a front end must never soften: the "
               "node limit was reached, so neither answer was established.",
        "tool": "decide-rank",
        "fixture": "f2_5x5.tensor",
        "options": {"--target": "13", "--node-limit": "1000"},
        "expect": "undecided",
    },
    {
        "title": "25 multiplications down to 14, cheaply",
        "why": "The descent, which only ever claims an upper bound. Its exit 0 "
               "says what it reached and nothing about the rank.",
        "tool": "minimise-rank",
        "fixture": "f2_5x5.tensor",
        "options": {},
        "expect": "reached",
    },
    {
        "title": "The rank as a factorisation, with a receipt",
        "why": "S = C A with every row of A rank one, and the check that C A is "
               "the map, which anybody can multiply out by hand.",
        "tool": "factor-over-canonical-basis",
        "fixture": "f2_2x3.tensor",
        "options": {},
        "expect": "factored",
    },
    {
        "title": "Fourteen products, and then fewer nonzeros in each",
        "why": "README.md's pipeline in one press: the descent's operators "
               "handed straight to the sparsifier, 31 nonzeros down to 27. Four "
               "runs, four commands, and no claim about the rank in any of them.",
        "flow": "decompose-then-sparsify",
        "fixture": "f2_5x5.tensor",
        "expect": "complete",
    },
]
