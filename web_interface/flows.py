"""The questions this console answers with more than one tool, in order.

`catalogue.py` is the table of tools and every one of them answers on its own.
This is the other kind, and `README.md` already documents one: the rank search
recovers the operators and sparsification is what they are for, so the second
question cannot be asked until the first has been answered. Typed at a terminal
that is two lines; here it was four presses and a file path copied by hand
between them, which is where the path stopped being the one the search wrote.

**A flow adds no binary, no flag and no mathematics.** The whole of what it adds
is the order. Every step is an ordinary run of an ordinary tool, with its own
command, its own card and its own exit code, so a flow cannot claim anything its
steps did not and `complete` below is not a verdict: it says every step ran and
ended where its own tool says it ends, and each card carries what that step
decided.

`expect` is the badge a step has to reach for the flow to go on, in the tool's
own word from `catalogue.py`. A step that ends any other way stops the flow
where it is, because the tool after it would otherwise be handed a path that was
never written, and a sparsifier refusing to open a file reads on a card as
though the sparsifier were broken.

`takes` is the suffix a step reads from what the step before it wrote. It
matches against the files that are really there rather than against a
transcription of what the tool promises: `minimise-rank --emit-operators <stem>`
writes `<stem>_{L,R,P}.sms` today, and a flow naming those three would go on
naming them after the tool stopped writing one.
"""

FLOWS = [
    {
        "name": "decompose-then-sparsify",
        "input": "tensor",
        "asks": "How few multiplications, and then how few nonzeros in each of "
                "the operators that write them down?",
        "answers": "README.md's pipeline, run here rather than retyped: the "
                   "descent's upper bound and the <L, R, P> it emits, then the "
                   "least nonzeros a change of basis can leave in each of the "
                   "three. Four runs and four commands, and neither tool is "
                   "asked anything it is not asked on its own.",
        "steps": [
            {"tool": "minimise-rank",
             "options": {"--emit-operators": True},
             "expect": "reached",
             "says": "minimise-rank on the map, asked for its operators."},
            # No `--field p`. That flag asks the sparsifier the question over
            # GF(p), which is a different and easier one than the matroid greedy
            # over Q answers, and README.md's pipeline does not pass it. The
            # console runs the documented line; the flag is there on the tool
            # for anybody who wants the other question.
            {"tool": "sparsify-operator",
             "takes": ".sms",
             "options": {},
             "expect": "minimum",
             "says": "then sparsify-operator, once on each .sms the descent "
                     "wrote, over Q as README.md's second line runs it."},
        ],
    },
]

BY_NAME = {flow["name"]: flow for flow in FLOWS}
