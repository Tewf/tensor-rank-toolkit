"""The tools this console offers, what each one asks, and the flags it takes.

**The list is the interface, so it is checked against the build rather than
counted.** `tests/catalogue_against_the_build.py` asserts in both directions and
at both depths: every binary named here exists and every command the build
produces is either here or named there as not a tool, and every flag a binary's
own `--help` prints is either an option below or named there as deliberately not
offered. A literal count is what let the first of those drift: `lower-the-bound`
shipped on 2026-08-21 and did not reach this file for a day, while
`len(TOOLS) == 12` stayed true throughout. Nothing at all was watching the
second, and twenty-one flags had reached the binaries and not this table by
2026-08-23, of which nineteen belonged here. Among those nineteen was
`sparsify-operator --field p`, the only route to the exact answer over GF(p) and
so the only route to the question an operator the rank search emitted is actually
asking. Which command answers what, and which binaries are deliberately not tools:
`../OPTIONS/one-question-per-command.md`.

A table and not behaviour: every entry is transcribed from the tool's own
`--help` and from `OPTIONS.md`, which is where a flag's default and the
measurement behind that default are written down. Nothing here restates a
default. A field left out is a field the tool decides, and `OPTIONS.md`'s
precedence then applies unchanged: an explicit flag, else `tunables.conf`, else
the number compiled in.

`verdicts` is the one place a tool's own reading of exit 0 and exit 1 is
recorded, because the tools do not all answer the same question with them and
`infrastructure/cli/exit_code.h`'s sentence is the standing claim rather than the tool's
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

# One spelling, and what it changes is a fact about each tool. Every one of them
# moves the wall clock; only a tool that stops at a budget can have an answer
# moved, because a witness found by one worker stops the others and what they
# have already spent counts against the same limit. That warning was written
# once and shared, so a flip walk and a counting enumeration carried a sentence
# about an exit code neither of them can reach. Written beside each instead, for
# the reason the memory note above gives.
THREADS = {"flag": "--threads", "kind": "count", "label": "threads",
           "note": "0 for every core, 1 by default."}


def threads(changes):
    """`--threads` on one tool, with what more of them change there."""
    return dict(THREADS, note=THREADS["note"] + " " + changes)


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

# The same spelling on the six tools where it decides no pool: still the bulk
# allocation budget in bytes, and still the one number that decides whether a run
# can start at all. What it refuses differs a tool at a time, so that sentence is
# written beside each rather than shared, which is the lesson above.
ALLOCATION_MEMORY = {"flag": "--max-memory", "kind": "memory",
                     "label": "memory cap",
                     "note": "Bytes one bulk allocation may take, 2G by "
                             "default. Written as a size, 2G or 2048M.",
                     "budget": True}


def memory_cap(refuses):
    """`--max-memory` on one of those tools, with what it refuses there."""
    return dict(ALLOCATION_MEMORY,
                note=ALLOCATION_MEMORY["note"] + " " + refuses)

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
                     "four cases and cost one to two orders of magnitude "
                     "more than steps 1 and 2 together: fixtures/README.md."},
            {"flag": "--emit-operators", "kind": "emit_operators",
             "label": "write the operators (L, R, P as SMS)",
             "note": "Three .sms files in the format PLinOpt reads."},
            {"flag": "--plateau", "kind": "count", "label": "equal-cost steps",
             "note": "The crossing runs on one core whatever the thread count "
                     "says, and the arithmetic is in "
                     "infrastructure/run_limits/adapting-to-the-machine/what-was-closed.md."},
            {"flag": "--plateau-states", "kind": "count",
             "label": "states per plateau crossing", "budget": True,
             "note": "From plateau_state_budget, 200000. Measured and left "
                     "there: <2,2,2> crosses to 7 at 380 states and stays at 8 "
                     "at 370, and 380 is one shape's answer rather than a "
                     "rule. flip_graph/results.json."},
            {"flag": "--json", "kind": "switch", "label": "results as JSON"},
            dict(SYMMETRY),
            threads("The three descent steps adopt the same candidates in the "
                    "same order at any count, so only the wall clock moves. "
                    "The plateau crossing above does not read it at all."),
            dict(MEMORY),
        ],
    },
    {
        "name": "lower-the-bound",
        "binary": "incumbent_search/lower-the-bound",
        "input": "tensor",
        "asks": "How much cheaper can a branch and bound make an algorithm?",
        "answers": "The exact search's tree cut by what has been built rather "
                   "than by a target, so it finds and never refutes. Every "
                   "count it prints was rebuilt and multiplied out first.",
        "verdicts": {"0": {"badge": "reached",
                     "means": "the search ran and reported the cheapest verified "
                              "algorithm it holds. Nothing here refutes, so a "
                              "spent budget is a weaker answer and not no answer."}},
        "options": [
            {"flag": "--from", "kind": "choice", "values": ["basis", "descent"],
             "label": "root and first incumbent",
             "note": "descent by default, because a loose incumbent bounds "
                     "nothing. basis is a looser incumbent and therefore a "
                     "taller tree, which is what reaches 13 on f2_5x5 where "
                     "descent exhausts at 14: "
                     "incumbent_search/what-it-reaches.md."},
            {"flag": "--below", "kind": "count",
             "label": "stop at k products or fewer",
             "note": "The incumbent is seeded at k+1 instead of at the start, so "
                     "the bound cuts at dimension k straight away and the tree "
                     "is smaller. Not reaching k refutes nothing: nothing here "
                     "does."},
            {"flag": "--nodes", "kind": "count", "label": "subspaces to expand",
             "budget": True,
             "note": "20000 by default. Spending it withdraws no answer."},
            {"flag": "--width", "kind": "count", "label": "children entered per node",
             "note": "4 by default, cheapest first. 0 enters every child, "
                     "which is the branch and bound rather than a beam and is "
                     "affordable on matmul_2x2x2 and nothing larger here."},
            {"flag": "--cost-drop", "kind": "count",
             "label": "most one move may take off the cost",
             "note": "0 by default, which leaves the bound as it was. A positive "
                     "s tightens it from dim+1 to the least value of "
                     "max(dim+t, cost-s*t), which at <3,4,5> is 38 rather than "
                     "16. Measured at 1 and not proved, so every child is "
                     "checked against it and a violation refuses the answer."},
            {"flag": "--summand-rank", "kind": "count",
             "label": "largest rank a move may split",
             "note": "3 by default. An element of rank r offers "
                     "(p^r - 1)p^(r-1)/(p-1) moves, so 6 at rank 2, 28 at 3 "
                     "and 120 at 4 over GF(2)."},
            {"flag": "--rounds", "kind": "count", "label": "restarts from the answer",
             "budget": True,
             "note": "8 by default, stopping as soon as a round does not "
                     "improve. It paid on nothing measured here."},
            {"flag": "--whole-pool", "kind": "switch",
             "label": "offer every rank-one map instead of generated moves",
             "note": "|pool| minimum-weight bases a node: 16 129 at 7x7 over "
                     "GF(2). Not measured on anything it could finish."},
            {"flag": "--span-census", "kind": "switch",
             "label": "count the subspaces this run reaches twice",
             "note": "Off by default and it changes nothing: a run with it on "
                     "enters the same tree and prints the same counts. What the "
                     "number is evidence about is whether isomorph rejection "
                     "could pay here at all."},
            {"flag": "--orbit-moves", "kind": "switch",
             "label": "one move per orbit instead of every move",
             "note": "Under the group -s names. Off by default: every count "
                     "published for this search was taken without it, and no "
                     "group is available for most fixtures here."},
            {"flag": "--emit-operators", "kind": "emit_operators",
             "label": "write the operators (L, R, P as SMS)",
             "note": "The same three .sms files minimise-rank writes."},
            {"flag": "--general-span", "kind": "switch",
             "label": "walk every span by the general field path",
             "note": "Off by default. Over GF(2) step 1 walks the span with a "
                     "bit an entry; this forces the general one, same tree and "
                     "same counts, so the two can be timed on one question: "
                     "descent_search/gf2_span_walk.h."},
            dict(SYMMETRY),
            threads("The children of one node are prepared in parallel and "
                    "entered in the same order at any count, so every number "
                    "this prints is what one worker printed."),
            memory_cap("--summand-rank r asks for p^r vectors, and this is what "
                       "refuses an r the machine cannot hold."),
        ],
    },
    {
        "name": "decide-rank",
        "binary": "exhaustive_search/decide-rank",
        "input": "tensor",
        "asks": "Is there an algorithm with k products, and is that proved?",
        "answers": "An exhaustive search. Exit 0 is a verified decomposition, "
                   "exit 1 is a proof that none exists, exit 3 is a budget "
                   "that ran out and proves nothing. Hand it two slices and it "
                   "does not search at all: a pencil has a complete invariant, "
                   "so the Kronecker form settles those in polynomial time and "
                   "this takes that answer wherever the form is exact.",
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
            {"flag": "--device", "kind": "choice",
             "values": ["auto", "cpu", "gpu"],
             "label": "which processor answers a leaf",
             "note": "auto is the default and takes the card where one is "
                     "compiled in, present, has a kernel for the shape, and the "
                     "leaf is over device_launch_floor elements. gpu asks for it "
                     "and lifts that floor; cpu takes it off the table. gpu is a "
                     "request and not an instruction, and the device: line in "
                     "the plan says which one answered and why."},
            {"flag": "--plan-out", "kind": "emit_file", "suffix": "_plan.txt",
             "label": "write the seven choices this run made",
             "note": "The run carries on. Replayed elsewhere it is "
                     "--plan-in, which names a second file to read and is "
                     "therefore not offered here, so the card below is where "
                     "you fetch this one and carry it to a terminal."},
            {"flag": "--trace", "kind": "emit_file", "suffix": "_trace.jsonl",
             "label": "write what the search walked (JSON Lines)",
             "note": "One line per node opened, bounded, pruned or adopted, so "
                     "the file grows with the tree rather than with the answer. "
                     "It needs one worker: two interleave their nodes and what "
                     "comes out is not a tree, so the run refuses the pair "
                     "rather than writing one."},
            dict(SYMMETRY),
            threads("A refutation visits the same nodes at any count. A "
                    "satisfiable question does not: a witness stops the "
                    "workers already running and what they spent counts "
                    "against the same budget, so a tight node limit can turn "
                    "exit 0 into exit 3. "
                    "exhaustive_search/what-threads-change.md."),
            dict(MEMORY),
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
            # Its own note again, and for the same reason: nothing here has a
            # node limit, and what a second worker changes is which cube an
            # answer comes back from and how the memory cap is divided.
            {"flag": "--threads", "kind": "count", "label": "threads",
             "note": "0 for every core, 1 by default. -s matmul splits the "
                     "question into one instance per orbit of the first term, "
                     "and those are independent solver processes; each worker's "
                     "cap is the cap below divided by the workers, so the "
                     "aggregate ceiling is the one number the flag names. A "
                     "refutation is the same at any count; a yes may come back "
                     "from a different cube."},
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
            threads("The seeds are independent walks, each reproducible from "
                    "its own seed number, so this moves the wall clock and "
                    "nothing else: same seeds, same schemes, same output."),
            memory_cap("--from k runs the heuristic first, whose span table is "
                       "p^dim, so this is what refuses a shape the machine "
                       "cannot hold."),
        ],
    },
    {
        "name": "decide-rank-by-pencil",
        "binary": "pencil_rank/decide-rank-by-pencil",
        "input": "tensor",
        "asks": "Two slices: what does the Kronecker canonical form say?",
        # The one shape limit any tool here has, declared rather than left to
        # the binary to refuse after Run. A pencil is `A + xB`, and the whole
        # method is that two slices have a complete invariant where three do
        # not, which is where Hastad's NP-completeness starts.
        "most_slices": 2,
        "answers": "Two slices only, and for those it is polynomial time with "
                   "no candidate pool at all: Kronecker's minimal indices and "
                   "elementary divisors settle the pencil, so there is nothing "
                   "to search. It reports a proved lower bound, a sharper count "
                   "marked provisional, and the exact rank where the field is "
                   "large enough for the pencil or the pencil is diagonalisable "
                   "over it.",
        "verdicts": {"0": {"badge": "read off",
                     "means": "the canonical form was computed. Read which of "
                              "the three claims it makes: lower bound, "
                              "provisional, or exact."}},
        "options": [
            memory_cap("There is nothing to tune here and this is not a tuning: "
                       "the Sumi bound builds x^p - x, which is linear in the "
                       "characteristic the file names, so this is the one number "
                       "a caller may have to move to run at all."),
        ],
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
            {"flag": "--ceiling", "kind": "count", "label": "ceiling",
             "budget": True,
             "note": "The sum of the slices' ranks by default, which "
                     "decomposing each slice alone always reaches. Stopping "
                     "lower prices the sweep at a stated number of levels; "
                     "below the rank every level refutes and the run ends "
                     "undecided, which proves nothing about the rank."},
            {"flag": "--route", "kind": "choice",
             "values": ["auto", "exhaustive", "sat", "canonical"], "label": "route"},
            {"flag": "--node-limit", "kind": "count", "label": "node limit",
             "budget": True},
            dict(SYMMETRY),
            threads("The sweep is sequential and every level of it is the "
                    "search --route names, so what more workers change is what "
                    "they change there: the level that succeeds is satisfiable, "
                    "and a tight --node-limit on it can end the run undecided "
                    "instead of factored."),
            dict(MEMORY),
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
            dict(SYMMETRY),
            threads("This walk counts rather than stops, so there is no budget "
                    "to spend early against and no witness to stop anybody: "
                    "every number it reports is the same at any worker count."),
        ],
    },
    {
        "name": "sparsify-operator",
        "binary": "matrix_sparsification/sparsify-operator",
        "input": "operator",
        "asks": "How few nonzero entries can one operator be written with?",
        "answers": "The fewest any change of basis can leave, which the matroid "
                   "greedy over Q returns and nothing can beat.",
        "verdicts": {"0": {"badge": "minimum",
                     "means": "the least number of nonzeros over every "
                              "invertible V, by Rado-Edmonds. --simplex answers "
                              "the same question by linear programming and is an "
                              "upper bound rather than a proof; --operations "
                              "minimises nnz + nns instead, which is a different "
                              "question."}},
        "options": [
            {"flag": "--field", "kind": "count",
             "label": "field p, to answer over GF(p) instead of over Q",
             "note": "Reads the entries over GF(p) and answers there, exactly, "
                     "by the matroid greedy over the column space. .sms only. "
                     "The routes below work over Q, which is a different and "
                     "harder question, and an operator a rank search emitted is "
                     "over a finite field: this is the question it is asking.",
             # A minimum over GF(p) is not the minimum over Q, and the badge is
             # the same word for both. So the flag carries the reading, the way
             # `--emit-cnf` does above, rather than letting the answer be read as
             # the harder one it is not.
             "verdicts": {"0": {"badge": "minimum",
                          "means": "the least number of nonzeros over every "
                                   "invertible V over GF(p), by Rado-Edmonds. "
                                   "That is a different and easier question "
                                   "than the same minimum over Q, which is what "
                                   "this tool answers without the flag."}}},
            {"flag": "--simplex", "kind": "switch",
             "label": "answer by linear programming, the only route that finishes a large operator"},
            {"flag": "--operations", "kind": "switch",
             "label": "also minimise nnz + nns, where a 4/9 costs more than a 1"},
            # `_sparsified` and not the stem alone: this tool is the one whose
            # input and output are the same format, so a name built from the
            # stem and `.sms` would be the operator it was asked to read.
            {"flag": "--emit", "kind": "emit_file", "suffix": "_sparsified.sms",
             "label": "write the sparsified operator (SMS)",
             "note": "Written the way the file came in, so it drops in where "
                     "the original did: a decoding operator goes back up the "
                     "way it was given, which is the program that is run."},
            {"flag": "--show", "kind": "switch", "label": "print the matrix as well"},
            memory_cap("The scan is priced by the column supports it may walk "
                       "rather than by what it allocates: about ten megabytes "
                       "on a 23x9 operator and 1.4 PiB on a 49x16 one, which is "
                       "what refuses the second in milliseconds. --simplex "
                       "answers it."),
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
            {"flag": "--solvers", "kind": "switch",
             "label": "print the backends this machine has and stop",
             "note": "The integer programming backends in preference order, and "
                     "whether each is on PATH. Every solver here is optional "
                     "and found at run time. Was the command `list-solvers`.",
             "verdicts": {"0": {"badge": "listed",
                          "means": "the machine was asked which backends it "
                                   "has. Nothing was minimised."}}},
            memory_cap("The frontier is quadratic in --degree, so this is what "
                       "refuses a degree this machine cannot hold."),
        ],
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
        "options": [
            memory_cap("Every mode above is cubic in the numbers it takes, so "
                       "this is what refuses a shape the machine cannot hold."),
        ],
    },
]

BY_NAME = {tool["name"]: tool for tool in TOOLS}
