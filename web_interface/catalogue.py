"""The twelve tools, what each one asks, and the flags it takes.

A table and not behaviour: every entry is transcribed from the tool's own
`--help` and from `OPTIONS.md`, which is where a flag's default and the
measurement behind that default are written down. Nothing here restates a
default. A field left out is a field the tool decides, and `OPTIONS.md`'s
precedence then applies unchanged: an explicit flag, else `tunables.conf`, else
the number compiled in.

`verdicts` is the one place a tool's own reading of exit 0 and exit 1 is
recorded, because the twelve do not all answer the same question with them and
`cli/exit_code.h`'s sentence is the standing claim rather than the tool's
wording. `budget` marks a flag that bounds the run: the interface passes its own
wall clock through those where a tool has one, so the limit is named in the
command a reader can retype rather than imposed invisibly from outside.

Option kinds, all validated in `command_line.py` before anything is started:
`switch`, `count`, `whole`, `choice`, `memory`, `text`, `symmetry`, `points`,
`emit_operators`, `emit_file`.
"""

SYMMETRY = {"flag": "-s", "kind": "symmetry", "label": "symmetry",
            "note": "none tries every candidate and is the default; auto "
                    "quotients by the map's own stabiliser; matmul uses the "
                    "closed-form orbits of <n,m,k>."}

THREADS = {"flag": "--threads", "kind": "count", "label": "threads",
           "note": "0 for every core, 1 by default. More threads change the "
                   "wall clock, and on a satisfiable question a tight node "
                   "limit can turn exit 0 into exit 3: "
                   "exhaustive_search/what-threads-change.md."}

# One spelling, two meanings, which is a fact about the tools and not about this
# table: on the three searches it is the pool budget in bytes and decides whether
# the pool is held or addressed, and on the two that call a solver it is that
# solver's cap and comes from `sat_memory_megabytes`. The note said the second of
# both, so four of the five tools carrying it named a tunable that never reaches
# them. Written once each, since a shared note cannot say two things.
MEMORY = {"flag": "--max-memory", "kind": "memory", "label": "memory cap",
          "note": "The pool budget: under it the pool is held, over it every "
                  "element is derived from its index instead. Written as a size, "
                  "2G or 2048M.",
          "budget": True}

SOLVER_MEMORY = {"flag": "--max-memory", "kind": "memory",
                 "label": "memory cap on the solver",
                 "note": "Passed to the solver, from sat_memory_megabytes in "
                         "tunables.conf. Written as a size, 2G or 2048M.",
                 "budget": True}

TOOLS = [
    {
        "name": "minimise-rank",
        "binary": "descent_search/minimise-rank",
        "input": "tensor",
        "asks": "How few multiplications can a cheap descent reach?",
        "answers": "An upper bound, reached in three steps, with the encoding "
                   "operators if you ask for them. It never claims a minimum.",
        "verdicts": {"0": {"badge": "reached",
                     "means": "the descent ran and reported the upper bound it "
                              "reached. It is not a claim about the rank."}},
        "options": [
            {"flag": "--steps", "kind": "choice", "values": ["1", "2", "3"],
             "label": "steps",
             "note": "Step 3 enumerates the full pool. Across the four "
                     "polynomial fixtures it improved the answer in two of "
                     "four cases and cost 58 to 184 times steps 1 and 2 "
                     "together: fixtures/README.md."},
            {"flag": "--emit-operators", "kind": "emit_operators",
             "label": "write the operators (L, R, P as SMS)",
             "note": "Three .sms files in the format PLinOpt reads."},
            {"flag": "--plateau", "kind": "count", "label": "equal-cost steps"},
            {"flag": "--plateau-states", "kind": "count",
             "label": "states per plateau crossing", "budget": True,
             "note": "From plateau_state_budget. Never measured."},
            {"flag": "--json", "kind": "switch", "label": "results as JSON"},
            dict(SYMMETRY), dict(THREADS), dict(MEMORY),
        ],
    },
    {
        "name": "decide-rank",
        "binary": "exhaustive_search/decide-rank",
        "input": "tensor",
        "asks": "Is there an algorithm with k products, and is that proved?",
        "answers": "An exhaustive search. Exit 0 is a verified decomposition, "
                   "exit 1 is a proof that none exists, exit 3 is a budget "
                   "that ran out and proves nothing.",
        "verdicts": {"0": {"badge": "yes",
                     "means": "an algorithm with that many products exists, and "
                              "it was verified"},
                     "1": {"badge": "no",
                     "means": "there is no such algorithm, and the search was "
                              "exhaustive"}},
        "options": [
            {"flag": "--target", "kind": "count", "label": "products to test",
             "note": "Left out, it searches for the rank itself."},
            {"flag": "--anchor", "kind": "choice", "values": ["map", "heuristic"],
             "label": "anchor",
             "note": "map is the default and the answer is the true minimum. "
                     "heuristic is far cheaper and the answer is the minimum "
                     "only among algorithms containing that subspace."},
            {"flag": "--node-limit", "kind": "count", "label": "node limit",
             "budget": True,
             "note": "From search_node_limit. Reaching it is exit 3 and proves "
                     "nothing either way."},
            {"flag": "--leaf-limit", "kind": "count", "label": "leaf limit",
             "budget": True,
             "note": "From search_leaf_limit. The node limit bounds how many "
                     "leaves are reached and nothing inside one."},
            {"flag": "--general-leaf", "kind": "switch",
             "label": "general field path at every leaf",
             "note": "Same tree, same nodes, same answer, and slower."},
            {"flag": "--leaf-route", "kind": "choice",
             "values": ["auto", "scan", "walk"],
             "label": "leaf route",
             "note": "auto takes the cheaper by count and is right on every "
                     "question measured. scan and walk force one, for timing "
                     "them against each other on a single question."},
            {"flag": "--orbit-test", "kind": "choice",
             "values": ["full", "generators"],
             "label": "how -s rejects a repeated branch",
             "note": "Only read when -s is given. full keeps the least member "
                     "of each orbit and walks the orbit to find it; generators "
                     "tests only the images under the surviving generators, "
                     "which is cheaper a candidate and leaves duplicate "
                     "branches standing. Same verdict either way, and the node "
                     "counts say what the duplication costs: "
                     "orbit_reduction/what-partial-rejection-leaves.md."},
            dict(SYMMETRY), dict(THREADS), dict(MEMORY),
        ],
    },
    {
        "name": "decide-rank-by-sat",
        "binary": "satisfiability/decide-rank-by-sat",
        "input": "tensor",
        "asks": "The same question, put to somebody else's solver.",
        "answers": "With no range it sweeps upward and the first k it can "
                   "decompose into is the rank. A refutation can be written as "
                   "DRAT and checked by drat-trim.",
        "verdicts": {"0": {"badge": "yes",
                     "means": "the solver found a decomposition"},
                     "1": {"badge": "no",
                     "means": "the solver refuted it. With --proof the "
                              "refutation is DRAT and drat-trim can check it"}},
        "options": [
            {"flag": "--target", "kind": "count", "label": "products to test"},
            {"flag": "--from", "kind": "count", "label": "sweep from"},
            {"flag": "--to", "kind": "count", "label": "sweep to"},
            {"flag": "--ceiling", "kind": "count", "label": "upper bound to start from"},
            {"flag": "--backend", "kind": "choice", "values": ["cnf", "smt"],
             "label": "backend",
             "note": "cnf encodes the field into clauses and is the default; "
                     "smt hands GF(p) to cvc5's theory of finite fields."},
            {"flag": "--solver", "kind": "text", "label": "pin a solver"},
            {"flag": "--tune", "kind": "choice", "values": ["sat", "unsat"],
             "label": "kissat configuration",
             "note": "For a question whose answer you expect. No default until measured."},
            {"flag": "--break-symmetry", "kind": "switch", "label": "break symmetry",
             "note": "Sound, off by default, worth at least 76x on a question "
                     "expected to answer no."},
            {"flag": "--plain-cnf", "kind": "switch", "label": "expand parities into clauses"},
            {"flag": "--proof", "kind": "emit_file", "suffix": ".drat",
             "label": "write a DRAT refutation",
             "note": "kissat only. Any other solver, and the smt backend, "
                     "refuse the flag rather than write nothing."},
            {"flag": "--emit-cnf", "kind": "emit_file", "suffix": ".cnf",
             "label": "write the question and stop",
             # This flag changes what leaving with 0 means: the run wrote a file
             # and stopped, and no solver was asked. Reporting it as the tool's
             # ordinary exit 0 would announce a decomposition nobody looked for.
             "verdicts": {"0": {"badge": "written",
                          "means": "the question was written to a file and the "
                                   "run stopped. No solver was asked, and "
                                   "nothing about the rank was decided."}}},
            {"flag": "--timeout", "kind": "count", "label": "seconds per question",
             "budget": True, "carries_wall_clock": True,
             "note": "From sat_timeout_seconds. This is also the alarm the "
                     "solver carries, so it bounds what outlives a stop."},
            {"flag": "--probe", "kind": "count", "label": "seconds for questions on the way",
             "budget": True},
            dict(SYMMETRY), dict(SOLVER_MEMORY),
        ],
    },
    {
        "name": "walk-scheme",
        "binary": "flip_graph/walk-scheme",
        "input": "tensor",
        "asks": "Can a decomposition be moved sideways into a smaller one?",
        "answers": "A flip-graph walk. Every seed is an independent walk and is "
                   "reproducible from its own seed number.",
        "verdicts": {"0": {"badge": "reached",
                     "means": "the walk ran and reported the best scheme it "
                              "reached. It is not a claim about the rank."}},
        "options": [
            {"flag": "--flips", "kind": "count", "label": "flips per seed",
             "budget": True, "note": "20000 by default."},
            {"flag": "--seeds", "kind": "count", "label": "independent walks",
             "budget": True, "note": "8 by default."},
            {"flag": "--from", "kind": "count", "label": "start from a k-product scheme",
             "note": "The heuristic has to reach k or fewer or the run refuses."},
            dict(THREADS),
        ],
    },
    {
        "name": "decide-rank-by-pencil",
        "binary": "pencil_rank/decide-rank-by-pencil",
        "input": "tensor",
        "asks": "Two slices: what does the Kronecker canonical form say?",
        "answers": "Polynomial time and no candidate pool. It reports a proved "
                   "lower bound, a sharper count marked provisional, and exact "
                   "only where the pencil is diagonalisable over the field.",
        "verdicts": {"0": {"badge": "read off",
                     "means": "the canonical form was computed. Read which of "
                              "the three claims it makes: lower bound, "
                              "provisional, or exact."}},
        "options": [],
    },
    {
        "name": "factor-over-canonical-basis",
        "binary": "canonical_factorisation/factor-over-canonical-basis",
        "input": "tensor",
        "asks": "The rank as the factorisation it is, with a receipt.",
        "answers": "A over the canonical basis with every row a rank-one "
                   "matrix, and the C with C A equal to the tensor's slices. "
                   "The row count of A is the rank when the sweep below it was "
                   "complete.",
        "verdicts": {"0": {"badge": "factored",
                     "means": "a factorisation was produced and checked. The row "
                              "count of A is the rank only where the sweep below "
                              "it was complete."}},
        "options": [
            {"flag": "--floor", "kind": "count", "label": "floor"},
            {"flag": "--route", "kind": "choice",
             "values": ["auto", "exhaustive", "sat", "canonical"], "label": "route"},
            {"flag": "--node-limit", "kind": "count", "label": "node limit",
             "budget": True},
            dict(SYMMETRY), dict(THREADS), dict(MEMORY),
        ],
    },
    {
        "name": "deflate-strictly",
        "binary": "oracle_guided_search/deflate-strictly",
        "input": "tensor",
        "asks": "Can a committed candidate be refuted from the tree?",
        "answers": "Exit 0 accepts the candidate, exit 1 refutes it, exit 3 is "
                   "a budget that ran out.",
        "verdicts": {"0": {"badge": "accepted",
                     "means": "the candidate was accepted"},
                     "1": {"badge": "refuted",
                     "means": "the candidate was refuted"}},
        "options": [
            {"flag": "--target", "kind": "count", "label": "rank to test against",
             "required": True},
            {"flag": "--refuter", "kind": "choice", "values": ["solver", "tree"],
             "label": "refuter",
             "note": "solver waits for unsatisfiable on each cube; tree walks "
                     "the span enlarged by the candidate. Default solver."},
            {"flag": "--candidate-timeout", "kind": "count",
             "label": "seconds per candidate", "budget": True,
             "carries_wall_clock": True,
             "note": "Solver route only. The tree route is bounded by nodes."},
            {"flag": "--node-limit", "kind": "count", "label": "node limit",
             "budget": True},
            {"flag": "--solver", "kind": "text", "label": "pin a solver"},
            {"flag": "--parallel", "kind": "switch", "label": "ask on all cores",
             "note": "Prices every candidate rather than stopping at the first yes."},
            {"flag": "--break-symmetry", "kind": "switch", "label": "order terms 1 onward"},
            dict(SYMMETRY), dict(SOLVER_MEMORY),
        ],
    },
    {
        "name": "enumerate-subspaces",
        "binary": "oracle_guided_search/enumerate-subspaces",
        "input": "tensor",
        "asks": "How many solution subspaces are there, once per orbit?",
        "answers": "A count, not a decision. It walks and counts, so every "
                   "number it reports is the same at any worker count.",
        "verdicts": {"0": {"badge": "counted",
                     "means": "the enumeration completed. This walk counts "
                              "rather than stops, so the numbers are the same at "
                              "any worker count."}},
        "options": [
            {"flag": "--target", "kind": "count", "label": "dimension to enumerate up to",
             "required": True},
            {"flag": "--plain", "kind": "switch", "label": "orderings deduplicated only"},
            {"flag": "--canonical", "kind": "switch",
             "label": "McKay canonical augmentation, one per orbit",
             "note": "Both passes run when neither is named."},
            dict(SYMMETRY), dict(THREADS),
        ],
    },
    {
        "name": "sparsify-operator",
        "binary": "matrix_sparsification/sparsify-operator",
        "input": "operator",
        "asks": "How few nonzero entries can one operator be written with?",
        "answers": "Every method run on the one operator, so the output is the "
                   "comparison and nothing is chosen for you.",
        "verdicts": {"0": {"badge": "compared",
                     "means": "every method ran and reported what it reached. "
                              "The output is the comparison; nothing is chosen "
                              "for you."}},
        "options": [
            {"flag": "--show", "kind": "switch", "label": "print the matrix as well"},
        ],
    },
    {
        "name": "curve-bounds",
        "binary": "curve_bounds/curve-bounds",
        "input": "none",
        "asks": "What does interpolation on an algebraic curve bound?",
        "answers": "An envelope, not a bound on mu_sym_q(m): nothing here "
                   "checks that such a curve exists.",
        "verdicts": {"0": {"badge": "bounded",
                     "means": "the minimiser answered. This is an envelope, not "
                              "a bound on mu_sym_q(m)."}},
        "options": [
            {"flag": "--degree", "kind": "count", "label": "divisor degree",
             "note": "Spent exactly, not as a budget."},
            {"flag": "--points", "kind": "points", "label": "points available",
             "note": "Written d:n, one term per degree, for instance 1:5 2:3."},
            {"flag": "--route", "kind": "choice",
             "values": ["built-in", "chain", "enumeration"], "label": "route",
             "note": "built-in is exact branch and bound in rationals and its "
                     "optimum is a proof; chain asks the first installed MILP "
                     "backend; enumeration is the dynamic programme."},
            {"flag": "--node-limit", "kind": "count", "label": "node limit",
             "budget": True, "note": "Reaching it falls back to the dynamic programme."},
            {"flag": "--solver-timeout", "kind": "count",
             "label": "seconds for an outside solver", "budget": True,
             "carries_wall_clock": True, "note": "--route chain only."},
            {"flag": "--table", "kind": "switch",
             "label": "print [rambaud2014, Table 1] and stop",
             "verdicts": {"0": {"badge": "printed",
                          "means": "the transcribed table was printed and the "
                                   "run stopped. Nothing was minimised."}}},
        ],
    },
    {
        "name": "list-solvers",
        "binary": "integer_programme/list-solvers",
        "input": "none",
        "asks": "Which backends does this machine have?",
        "answers": "The integer programming backends in preference order, and "
                   "whether each is on PATH. Every solver here is optional and "
                   "found at run time.",
        "verdicts": {"0": {"badge": "listed",
                     "means": "the machine was asked which backends it has"}},
        "options": [],
    },
    {
        "name": "make-tensor",
        "binary": "map_construction/make-tensor",
        "input": "none",
        "produces": "tensor",
        "asks": "Build a map to run the others on.",
        "answers": "A tensor file, which this interface puts straight into the "
                   "map box above.",
        "verdicts": {"0": {"badge": "built",
                     "means": "the map was written, and is now in the box above"}},
        "modes": [
            {"flag": "--polynomial", "label": "polynomial multiplication",
             "fields": ["field p", "left terms", "right terms"]},
            {"flag": "--matmul", "label": "matrix multiplication <n,m,k>",
             "fields": ["field p", "n", "m", "k"]},
            {"flag": "--cyclic", "label": "cyclic convolution",
             "fields": ["field p", "length"]},
            {"flag": "--field", "label": "multiplication in GF(p^d)",
             "fields": ["field p", "modulus coefficients, highest degree first"],
             "trailing": True},
        ],
        "options": [],
    },
]

BY_NAME = {tool["name"]: tool for tool in TOOLS}
