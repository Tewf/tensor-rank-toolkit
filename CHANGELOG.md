# Changelog

All notable changes to this project are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the version
numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **`start-here.md`, a task-first front door in plain words**: the visitor
  who arrives with one want (a tensor and its decomposition) and none of
  the field's vocabulary now gets the file format shown whole, the one
  command per want, and the three words defined where they are used, without
  the theory. Both READMEs point at it first. And every top-level folder now
  carries a README saying what is in the folder, with an example of use
  whose output was produced rather than typed; nine had none: `cli/`,
  `exhaustive_search/`, `map_construction/`, `OPTIONS/`,
  `oracle_guided_search/`, `run_limits/`, `search_plan/`, `testing/` and
  `tools/`.

- **`cmake --install` places the thirteen tools on PATH**, which every
  documented command line assumed and nothing provided: the first line a
  newcomer pasted from the README failed with command-not-found. The three
  instruments and the `list-solvers` shim deliberately stay in the build tree;
  the top `CMakeLists.txt` says why. Both READMEs carry the bridge, and the
  dependency line now names every package the build needs (`pkg-config`,
  `libgmp-dev` and `ninja-build` were missing; the Containerfile knew).

- **`.containerignore`**, so `podman build` is handed what a fresh clone
  carries instead of half a gigabyte of build trees and `.git` that
  `COPY . /src` then baked into the reproduction image.

- **`reproduce/notice_scope.py`, in CI**: NOTICE's promise that its MIT list
  is every directory the build adds was kept by hand and drifted twice; it is
  now compared against `CMakeLists.txt`, and NOTICE's count of PLinOpt's
  matrices against the directory. Found by the same audit: the count said
  eight, five documents said twelve, and the directory holds thirteen, since
  the five that arrived with the interchange work joined the eight already
  there; `incumbent_search/` and `search_plan/` were unclaimed.

- **`reproduce/README.md`** and an import recipe in
  `OPTIONS/common-recipes.md`: the two pages simulated first-contact users
  reached for and did not find: the reviewer's one command, and the line
  that reads a published ⟨L,R,P⟩ triple in.

### Fixed

- **`show-limits` aborted on a typo**: no catch around `refuse()`, so an
  unrecognised flag reached `std::terminate` and dumped core (reachable from
  the browser console, which shells out to it), and its usage went to stdout,
  which is for results. **`measure-leaf` read `--help` as a mode it did not
  know**, fell through every branch and left green. Both now refuse by name
  and leave as 2, and `check_argument_grammar.sh` covers the instruments it
  had been claiming to cover, `measure-leaf` where nvcc built it.

- **`serve.py --host 0.0.0.0` produced a console that answered nobody**: the
  wildcard went into the allowed-hosts set, so every remote request's Host
  header failed the guard. A wildcard is refused at startup with the working
  routes spelled out; a named `--host` already worked. The web README's
  hardcoded "64 checks" is now owned by the run itself, which counts the
  README's sentence against what ran.

- **`cli/arguments.h` promised stdin nobody implemented**: the header and its
  worked example said a lone `-` means stdin, while all thirteen callers
  answer it with usage and exit 2. The accessor is renamed `no_file_named`,
  and the header now says nothing reads stdin.

### Changed

- **The sparsification result is stated on its own terms, and stops being
  framed as a contest.** The front page, both READMEs, `what-it-computes.md`,
  the strand's positioning note and the article all presented `Grey-221` at 128
  nonzeros as a win over another tool's 167. The margin was never the claim
  that carries weight: **128 is the minimum over every invertible change of
  basis**, proved by Rado-Edmonds, and it says the same thing with nothing to
  beat. So the 167 column leaves `index.html`'s table and the article's Table 2,
  both of which are now as given against minimum, and the article's four
  qualifications become three, the one about two tools doing different jobs
  having lost its subject.

- **No measurement was deleted and no credit moved.** `against-plinopt.md` and
  `in-front-of-plinopt.md` were two pages of one record, both titled as a
  contest; they merge into
  [`matrix_sparsification/measured-with-other-tools/`](matrix_sparsification/measured-with-other-tools/README.md),
  a folder of three pages under the markdown ceiling with a linking README, which
  keeps every number, the carried column, the transposition of `P`, the equal
  effort given to each side and the reversal on one operator in nine.
  `results.json` keeps all three columns, under a `published_scheme_operators`
  block renamed from `against_plinopt` and still named in `questions.CARRIED`
  with its price. The `NOTICE`, the vendored `fixtures/plinopt/` operators and
  their CeCILL-B licence are untouched; `references.md` keeps the citation, the
  authorship and the licence byte for byte and loses only the two clauses that
  invited a comparison; and the bibliography still cites PLinOpt where the
  article attributes model (c).

- **The checkers moved with the pages that carry the numbers**, in the same
  commits: `reproduce/front_page.py` now holds `index.html` to the two columns
  it prints, and `reproduce/quoted_numbers.py` holds the article's table row to
  the two totals it shows. Neither ever asserted a number on a page that had
  stopped printing it.


## [0.4.1] - 2026-08-23

A patch release for one reason: v0.4.0 was tagged an hour before the
measurement that corrects its sharpest number, so the release naming the
finished state did not contain the correction, and the outward surfaces quoting
**2.30x** were quoting something no tag held. Everything below was on `main` and
unreleased. Nothing here changes an interface, and no count moved.

### Changed

- **The one timing this repository published as a ceiling is measured, and the
  ceiling was worth an order of magnitude.** Refuting eight products on GF(16)
  exhaustively stood at **2328 s**, a figure older than the count beside it: it
  was taken before the GF(2) bit-packed leaf existed, and it survived two
  regenerations because `measure.py` skips that question by default at 4.1
  minutes of one core, and because the only replacement runs anybody had were
  taken on a machine with a browser open, which `MEASURING.md` does not accept.
  `measure.py --slow` asked it properly on 2026-08-23: **247.118 s**, at the
  same 105 600 301 nodes to the digit. The projection that stood in for it
  guessed the shipped binary at about six times faster; it is **9.4 times**.

  **The number it was propping up was the comparison against a SAT solver.**
  `satisfiability/measurements.md` had the solver ahead by 21.8x on the hardest
  question here, marked as a ceiling; it is ahead by **2.30x**. The standing
  claim survives — the solver's advantage grows with the instance, from 11.7x
  behind on `⟨2,2,2⟩` to 2.3x ahead on GF(16) — but it is a slope rather than a
  cliff, and eight documents were quoting the cliff.

- **Two ratios stopped being quoted to three digits, by this repository's own
  rule.** The orbit quotient's seconds read 29.6x after one regeneration and
  29.5x after the next, off a quotiented run of 0.000992 s. `MEASURING.md` says
  a ratio whose smaller end is under a millisecond is an order of magnitude and
  not two digits, so it is **about 30x** now, and the per-node surcharge is
  **about 1.1x to 1.3x**. The node columns, which do not move, are what those
  pages argue from.

### Fixed

- **The front page said the rank of `f2_5x5` was open, two days after this
  repository closed it.** It printed `>= 13`, "what is proved here is
  `13 <= rank <= 14`" and "finding a thirteen is what would close it and has not
  been run", while `the-research-front/`, `incumbent_search/`,
  `what-it-computes.md` and the results file all said **rank = 13**, settled on
  2026-08-21 by the exhaustion refuting 12 and `lower-the-bound` exhibiting a
  13 in 80 nodes. The sentence is corrected where it originates, in the carried
  `exact_search.bounded` block of `descent_search/results.json`, which now
  carries `settled` beside the refutations it re-derives.

  **`reproduce/front_page.py` was green through all of it because it never read
  that table.** It reads it now, out of the three blocks that settle a map, and
  reads the sentence under it. Two things learned in the wiring: `&ge; 13` is
  indistinguishable from 13 once the entities are dropped, so an inequality in a
  column headed "Exact answer" is now its own failure; and the "every published
  row is on the page" direction the other two tables check would be wrong here,
  because this table prints a selection.

- **The same measured pair was published as two different ratios.** Refuting six
  products on `⟨2,2,2⟩` read 11.7x in `satisfiability/measurements.md` and 11.6x
  in `oracle_guided_search/tree_refutation.h`. The cause was not rounding: that
  comment took the tree's second from `descent_search/results.json` and kissat's
  from `satisfiability/results.json`, and the same question is timed in both, so
  the ratio was assembled from two runs. Both ends now come from the one row
  that carries the pair, which is **11.7x**, and that comment's `0.338 s` is the
  `0.339 s` the file has held since the remeasurement.

- **`OPTIONS/one-idea-several-spellings.md` opened on twelve tools** where every
  other surface says thirteen. It is the page a reader consults when a flag
  means two things on two commands, which is the wrong page to be one tool out
  on.


## [0.4.0] - 2026-08-23

Seven branches came back and every number this repository publishes was measured
again from a clean tree. Nothing here decides a rank that was not decidable
before. What it adds is a second lever on the searches that had one, a way to see
what a search walked rather than only how much of it there was, checks under four
documents that nothing was checking, and a set of numbers that agree with each
other.

### Added

- **A cost-aware bound for the incumbent search, `--cost-drop s`, which refuses
  its own answer rather than trusting the constant it rests on.** The standing
  bound is `cost(W) >= dim(W)`, and the tree is deep exactly where cost dwarfs
  dimension: at `⟨3,4,5⟩`, dimension 15 against a naive 60, it first says
  anything forty-four levels down. If one move removes at most `s` then a
  descendant `t` levels down also satisfies `cost >= c − s*t`, which takes that
  root from 16 to **38**. **`s` is measured at 1 on seven fixtures and is not
  proved**, so every child's drop is compared against it and a run that saw a
  violation refuses to report its answer. `gf64_multiplication` produces one, at
  a drop of 2, which is the reason the guard is not decoration.

- **`--span-census`, which turns "would McKay canonical augmentation pay on this
  search" from an opinion into a count.** A reduced row echelon form is already
  the canonical name of a subspace, so counting how often one run reaches one
  span by two orders of adjunction needs no group and no canonisation. Child
  repeats run **0% to 28.3%** across five fixtures at a saving ratio of at most
  1.39, and the cheapest instrument that could detect the duplication costs 34 to
  40% of a child, so a parent test cannot pay for itself here. The one high
  reading, 40.6% on `gf64_multiplication`, is round two re-walking round one,
  which a parent test does not remove at all: two rounds are two trees with two
  roots. [`incumbent_search/what-the-tree-repeats.md`](incumbent_search/what-the-tree-repeats.md).
  `--orbit-moves` is what does remove most of it where a group exists, and it
  removes it **by removing the tree rather than by deduplicating it**: 2 251
  children to 304 on `matmul_2x2x2`, while what survives on `matmul_2x2x3` is
  more duplicated than what it replaced.

- **`decide-rank --trace FILE`: what the search walked, and not only how much of
  it.** Every count this repository publishes was a total. The trace is JSON
  Lines, one record per node opened, bounded, pruned or adopted, in the shared
  vocabulary, and a check asserts that what comes out is a tree and that it
  counts what the search counts. It **costs a null pointer test per node when it
  is off**, which is every run that does not ask for it, and it needs
  `--threads 1`: two workers interleave their nodes and what comes out of that is
  not a tree.

- **Why the incumbent search cannot be seeded from a published scheme, with the
  measurement that refuses it.** A `k`-product scheme arrives with cost `k` and
  dimension `k`, so `dim V + 1 >= best` fires before the first move is generated.
  Measured on this repository's own 19-product `gf64_multiplication` scheme and
  kept on `main` rather than on the archive branch, because a rejection whose
  evidence is elsewhere is a rejection nobody can check:
  [`incumbent_search/why-it-cannot-be-seeded.md`](incumbent_search/why-it-cannot-be-seeded.md).


- **The browser console offers the flags its tools actually take.** Twenty-one
  had reached the binaries without ever reaching
  [`web_interface/catalogue.py`](web_interface/catalogue.py), and nineteen of
  them belonged there: `sparsify-operator --field p`, which is the only route to
  the exact answer over GF(p) and so the only route to the question an operator
  the rank search emitted is actually asking; `--max-memory` on the six commands
  that had grown one, each carrying the sentence saying what it refuses *there*,
  since one note cannot say six things; `-s`, `--threads`, `--below`,
  `--cost-drop`, `--span-census` and `--orbit-moves` on `lower-the-bound`, which
  had shipped with eight of its fifteen; `decide-rank --device`, `--plan-out`
  and `--trace`; `decide-rank-by-sat --threads`;
  `factor-over-canonical-basis --ceiling`; and `sparsify-operator --emit`. Two
  are left off and say why: `decide-rank --plan-in` names a second file to read
  where this console offers one box, which is the limit that already keeps
  `operators-to-tensor` off the tool list, and `walk-scheme --steps` is the
  accepted older spelling of a flag already there. `--field` carries its own
  reading of exit 0 beside itself, because the badge is the word `minimum`
  either way and a minimum over GF(p) is not the minimum over Q.

- **The catalogue is checked against the build flag by flag**, in
  [`web_interface/tests/catalogue_against_the_build.py`](web_interface/tests/catalogue_against_the_build.py),
  which runs each binary's own `--help` and compares rather than keeping a
  second table beside the first. Three ways it can be wrong and all three are
  asked: a flag reached the binary and not the panel, a flag left the binary and
  stayed in the panel, and an exclusion outlived the flag it excused. The tool
  half of that check had shipped a day earlier and was green throughout the
  drift above, which is what a check one level too shallow does. The console's
  own suite is 64 checks.

- **The browser console runs `README.md`'s pipeline as one flow.**
  `minimise-rank --emit-operators` and then `sparsify-operator` on each of the
  three operators it wrote, from one press, declared in
  [`web_interface/flows.py`](web_interface/flows.py) and driven in
  `web_interface/flow_runner.py`. The paths are the ones the console chose, so
  the second tool reads what the first one really wrote instead of a path
  retyped between four presses. A flow adds no binary, no flag and no
  mathematics: every step is an ordinary run with its own command, its own card
  and its own exit code, and a step that does not reach the badge its flow names
  stops the flow rather than handing the next tool a file that is not there. It
  is also the seventh worked example.

- **The console's 64 checks run in CI**, where they ran nowhere before, on
  Python 3's standard library and nine seconds. It sits after the step that
  re-derives the published counts, because unlike `front_page.py` and
  `quoted_numbers.py` it needs the build.

- **The sparsification result is in the article.** The exactness theorem, the
  three operators of a published rank-23 `⟨3,3,3⟩` scheme at 221 nonzeros to 128
  against `[plinopt]`'s 167, and the four qualifications a count like that
  invites, including the one that goes the wrong way: minimising nonzeros can
  cost additions.

- **Two more checks over documents nothing was checking.**
  [`reproduce/front_page.py`](reproduce/front_page.py) now holds `index.html`'s
  sparsification table to `matrix_sparsification/results.json`, nine cells, three
  totals against the column sums rather than against a fourth measurement, and
  the sentence above the table.
  [`reproduce/quoted_numbers.py`](reproduce/quoted_numbers.py) grew a second kind
  of source: a results file asserts a constant at second hand, because
  `measure.py --check` re-derives it on every push. Five claims over eleven
  documents, and the eleventh is the article, whose Table 2 could drift silently
  because a LaTeX tabular carries no prose to match.

- **The tenth strand has a section, the folder tree has `search_plan/`, and the
  French page has what it was never given**: the leaf paragraph, the section on
  reading and writing the triples the field publishes, the browser console, and
  the clangd flags. `what-it-computes.md` opened on "Ten strands" and carried
  nine, and the one missing was the one `README.md` sends a reader there for.

### Changed

- **Step 1's span walk holds a GF(2) matrix as bits in machine words**, which is
  the representation
  [`exhaustive_search/gf2_leaf.h`](exhaustive_search/gf2_leaf.h) already gave the
  exact search's leaf, applied to the two functions the incumbent search spends
  its life inside. `lower-the-bound` costs one `minimum_weight_basis_with` per
  child, and each of those walks a span taking the rank of a matrix at nearly
  every element, so what a matrix is made of is what that search costs.
  **2.5x to 19.0x**, measured one question at a time with and without the new
  `--general-span`, in
  [`descent_search/gf2_span_walk.h`](descent_search/gf2_span_walk.h) with the
  spread explained and the protocol caveat attached: the machine was not quiet,
  so the ratios stand and the seconds are upper bounds.

  **No count moves, and that is a property of the code rather than a claim about
  two copies of it.** The ceiling, the floor under the unranked half, the sort
  and the order the greedy takes candidates in stay in
  `minimum_weight_basis.cpp`, once, templated on the representation; only a
  rank, a walk step, a dimension and a membership test are packed. Nodes,
  children, moves offered, improvements, branches bounded and the algorithm at
  the end are identical on both paths, and
  [`descent_search/tests/test_gf2_span_walk.cpp`](descent_search/tests/test_gf2_span_walk.cpp)
  asserts that entry for entry over six GF(2) fixtures and all three calls.
  GF(3), GF(5), a slice wider than 64 columns and a set of slices that do not
  share a shape reach none of it, checked rather than assumed.
  `reproduce/measure.py --check` reproduces every published count with the same
  four SKIPPED lines.

  One primitive was added under it, `gf2_rank` in
  [`linear_algebra/gf2_bits.h`](linear_algebra/gf2_bits.h): the leaf only ever
  needed to know whether a rank was one, and step 1 needs the number.

- **Every published number was re-measured from a clean tree, and not one count
  moved.** That is the protocol behaving rather than a result: counts are exact
  arithmetic. The timings did move, in one direction and on the searches only.
  The descent's step 3 fell **13x to 22x** across the four polynomial fixtures,
  `⟨2,2,2⟩` refuting six products went from 0.41 s to **0.029 s**, and the flip
  walk on `⟨3,3,3⟩` from 13.30 s to 11.44 s; the solver figures barely moved, as
  another program's figures should not. What changed under them is the GF(2) leaf
  and the reflected Gray walk of 2026-08-20, and the four results files now carry
  the commit they were taken at, a clean tree, and three repeats.

- **Three arguments were rewritten rather than renumbered**, because the new
  figure changed what they say. `famous_tensors/where-the-exact-search-stops.md`
  derived one rate across a table whose rows are three different eras, and now
  says which is which and that every projection in it is a ceiling.
  `satisfiability/measurements.md` called the tree and kissat *comparable* at
  0.41 s against 0.31 s, and **the tree is now ahead by 11.6x**, which makes its
  standing claim stronger rather than weaker: the table holds a loss and a win
  where it held a tie and a win, so the crossover is inside it.
  `oracle_guided_search/tree_refutation.h` argues from what pinning is worth,
  which is one session against itself and unaffected, and now says which of its
  four figures can be refreshed and which nothing re-measures.

- **A ratio whose smaller end is under a millisecond is quoted as an order of
  magnitude and not to two digits**, which [`MEASURING.md`](MEASURING.md) now
  states as a rule. The thermal band is a percentage of the run and shrinks with
  it; the fixed cost of starting a process does not. So the descent's step 3
  costs **one to two orders of magnitude** more than the steps before it, in
  nine documents and the article, where it read "36 to 189 times" and, in the
  article, "58 and 184". The computed range is kept beside the claim. A hand run
  moved one fixture's ratio from 189 to 85 with nothing changed in the code.

- **The ratios quoted anywhere are the ones the results files publish.** The
  orbit quotient's seconds are **29.6x** and its per-node surcharge 1.32x, in the
  six documents that had 27.8x and 1.41x; the front page had 27.8x too, and a
  22 779x that `quoted_numbers.py` refuses in the eight documents it does watch.
  Both ends of the `--break-symmetry` ratio were retaken, which turned out to
  cost eighty seconds: **24.81 s to 0.338 s, 73x**, where half of it had been
  called unrefreshable.

- **`plateau_state_budget` is measured, on the three surfaces that still called
  it a guess**: the audit page, the console's flag note, and `minimise-rank`'s
  own `--help`, which that note is transcribed from. `⟨2,2,2⟩` crosses to 7 at a
  380-state budget and stays at the naive 8 at 370. **The default stays at
  200 000 anyway**, because 380 is one shape's answer and `⟨2,2,3⟩` does not
  cross at 2 000, so tuning down to it would be fitting a constant to a single
  point.

- **Two archive branches became one.** `dominated-methods` held the slow
  sparsifiers and `rejected-experiments` held everything else that lost, which is
  two branches doing one job. The sparsifiers are now
  `retired/dominated_sparsifiers/` on `rejected-experiments`, which gained the
  index this repository never had, and every pointer moved before the empty
  branch went, including the two in C++ that a prose sweep had read past.

- **OPTIONS.md's opening claim is true again.** It says it documents every flag
  of every command with its default and the measurement behind that default, and
  it was behind on fourteen: all of `lower-the-bound`'s recent ones and the
  `auto` arm of `--width`, `decide-rank`'s `--device`, `--trace`, `--plan-out`
  and `--plan-in`, `sparsify-operator --field`, and `--threads` and
  `--max-memory` on two commands that had them and did not say so. Two rows also
  stopped being true, `--width` and `--cost-drop` being measured since they were
  last written up as not.

- **The console's shared `--threads` note says what threads change at each tool.**
  It warned that a tight node limit can turn exit 0 into exit 3, which is true of
  `decide-rank`, which it was written for, and false of `walk-scheme` and
  `enumerate-subspaces`, which have no budget to spend early against and no
  witness to stop anybody with. Split the way the `--max-memory` note beside it
  was split, for the reason that one records.

- **The GPU span-ranks seam was measured and refused, and three fixes came out of
  the attempt.** The seam is **0.02% of a node**, so the Amdahl ceiling on it is
  1.0002x and structural; that number is the refusal. What stayed: the five gates
  have a test that runs without a card, `CUDA_ARCHITECTURES native` actually
  applies now instead of being written into the cache by `enable_language(CUDA)`
  before the guard could see it, and `show-limits`, the one tool whose whole
  output answers what this machine has, can finally see the card. The fast suite
  was also run for the first time ever with the kernels compiled in, **76 of 76**,
  against an RTX 4060 Laptop.

- **`factored_lex_min` held two jobs and now holds one**, the canonical image,
  with the setwise stabiliser in its own file beside it and the pool action table
  pointing at the test that actually holds it. No behaviour change.

### Fixed

- **`measure.py` took its provenance once per file, inside the write loop**, so
  every results file after the first recorded the tree as the previous file's
  write had left it; and `tree_clean` came through a helper that folds empty
  output into `None`, so a clean tree and a git that did not answer arrived
  identically. **Every results file in this repository's history carries a
  `tree_clean` that meant nothing.** The four current ones are the first with a
  true one. Nothing downstream reads the field, so no published number depended
  on it.

- **A wall clock was standing in for a node count on one of the two front
  pages.** `what-it-computes.md` priced the orbit quotient at 28x on a refutation,
  which is a timing from before the leaf was rewritten twice, while `README.md`
  had moved to the node count for exactly that reason. It is 39.2x, which
  `measure.py --check` re-derives, with the 2.3x on a find beside it, because
  giving only the larger number implies the quotient pays everywhere.


## [0.3.0] - 2026-08-23

Three days across four strands, and the first release that had to withdraw
something: a sparsification answer that is the minimum over every change of
basis rather than the best found, a second search direction that hands back an
algorithm whenever it is stopped, a way in for somebody else's published
triple, and a published claim about a matroid that a decision procedure
refuted.

### Added

- **`sparsest_basis_over_the_rationals`: the minimum, and a proof of it.**
  `[gottlieb2010]`'s driver with an oracle that answers `[beniamini2020]`'s
  Problem 2.15, so Rado-Edmonds makes the assembled answer the least number of
  nonzeros any invertible `V` can leave. The counts did not move — 43 / 42 / 43
  on a published rank-23 `⟨3,3,3⟩` scheme were already minimal — but nothing
  here could say so, and the cheapest route to them was 86x to 112x slower.

- **`--simplex`, which answers without searching.** One continuous programme per
  coordinate against `integer_programme`'s exact rational simplex, no new
  dependency. It is the only route that answers `4x4x4_49_156_L`, at 100
  nonzeros in 0.34 s, and it reaches the proved minimum four to fifteen times
  faster than the search wherever the search can prove one. An upper bound, not
  a proof: `matrix_sparsification/method/answering-without-searching.md`.

- `matrix_sparsification/omega_validator.{h,cpp}`, extracted so that the routes
  that left could leave: it is `[beniamini2020, Def. 3.2]` and the rescaling
  greedy still calls it.

- The measurement against `[plinopt]` the module's README had asked for since it
  was written: 221 nonzeros to 128 against its 167. And the composition
  experiment in front of its subexpression pass, which shows **minimising
  nonzeros can cost additions**.

- **`operators-to-tensor`, and with it a way in for somebody else's algorithm.**
  Nobody publishes a tensor: PLinOpt's `data/` is 153 SMS operators in
  `stem_{L,R,P}` triples, `[fmm-catalogue]` publishes the same triple as Maple
  matrices, and the matrix multiplication tensor is determined by its three
  dimensions. So the `.tensor` format is this repository's alone, and a
  collaborator arrives holding three `.sms` files that `--emit-operators` could
  write and nothing here could read. The command takes its three filenames and
  its `-q` in the order `PMchecker` takes them, and the arithmetic is
  `map_computed_by` unchanged. That the two sides mean the same thing by
  ⟨L,R,P⟩ is now arithmetic rather than prose: his published Strassen rebuilds
  `matmul_2x2x2` entry for entry, his Karatsuba rebuilds `f2_2x2`, and his
  63-product ⟨3,4,7⟩ rebuilds that map, which the two square triples cannot
  check because a transposed slice rebuilds them correctly.
  [`formats/interchange/exchanging-files.md`](formats/interchange/exchanging-files.md)
  is the page for whoever has such files.

- **Thirteen of PLinOpt's own operators, vendored under CeCILL-B** in
  [`fixtures/plinopt/`](fixtures/plinopt/README.md) with a copy of that licence
  beside them, as Article 5.3.1 asks. Three were already there and neither
  `NOTICE` nor the directory said where they came from or under what terms.

- **A predicate that says whether canonical augmentation will pay, before the
  search runs.** `oracle_guided_search/canonical_route_price.h` weighs the node
  saving against the parent test from the characteristic, the shape, the pool
  size, the factored degree, the group order and the generator count.
  [`when-canonical-pays/`](oracle_guided_search/when-canonical-pays/README.md)
  has the derivation, the four operations priced by the new
  `price-canonical-route`, and the ten sweeps it was corrected against. The
  saving side has `[mckay1998, Thm 3]` under it and orbit counting bounding it;
  the price side has `[linton2004]` §3.4 for the canonical image, whose
  candidate bound `k|G|` is the binding one here and whose "polynomial in `n` for
  any `k`" criterion this presentation meets at every shape, and nothing at all
  for the setwise stabiliser, which is GI-hard and measured rather than bounded.

- **`factor-over-canonical-basis --ceiling k`**, so a sweep can stop short of a
  rank neither route can settle, and the run reports its node count and its
  canonical images whether or not it found anything. A refutation sweep used to
  report one sentence and no number, which made the two routes uncomparable at
  every shape above `<2,2,2>`.

- **`incumbent_search/` and the `lower-the-bound` command: the exact search's
  tree, cut by what has been built rather than by a target.** `[bdez2012]`
  Algorithm 2 walks subspaces containing `span(T)` and stops a branch at
  `dim V > k` for a `k` fixed in advance, so a spent budget yields nothing. The
  same tree stopped at `dim V + 1 >= best`, with `best` the cheapest `cost(V)`
  reached, yields an algorithm whenever it is stopped. The bound is admissible
  because `cost(V) >= dim V`, and the direction can only find, never refute,
  because `cost(V) <= b` exhibits a `b`-product algorithm and `cost(V) > b`
  exhibits nothing ([`descent_search/sorted_span.h`](descent_search/sorted_span.h)
  has the counterexample).

  Moves are generated from `V` rather than scanned out of the pool. Abel
  summation on the cost identity gives
  `cost(V + <g>) − cost(V) = maxrank − Σ_r e_r`, which is `1` generically, so
  every adjunction that does not simply cost one more lowers some element's
  level: `g` with `rank(v − g) = rank(v) − 1`. Those have the closed form
  `(C a)(bᵀ R)` with `bᵀa = 1`, so an element of rank 3 over GF(2) offers **28**
  candidates where the 7x7 pool offers 16 129.

  **`cyclic_f2_7` goes from 15 to 13 in 22 nodes**, which is its published rank,
  where the descent's step-3 shortlist is **0 of 16 129** and it cannot take a
  first step. `gf32_multiplication` goes 16 to 14 in 139 nodes.
  `matmul_2x2x2` reaches 7 with the tree exhausted, in 184 nodes.
  [`incumbent_search/what-it-reaches.md`](incumbent_search/what-it-reaches.md)
  has every count, and the wall it does not clear: `p^dim` rank computations per
  child, where `dim` is the quantity the search raises.

- **`fixtures/f5_3x3.tensor`**, polynomial multiplication of 3 coefficients by 3
  over GF(5), and the third characteristic in that directory. It ships for a
  test rather than for a number: the general-field leaf walks a subspace by its
  base-`p` digits and every other fixture is over GF(2) or GF(3), so a walk right
  at `p = 3` and wrong at `p = 5` had nothing to fail against. Its rank is 5,
  which `decide-rank` closes in one node.

- **`--leaf-route auto|scan|walk`**, so the two ways a leaf can be answered can
  be timed against each other on one question rather than on two. The rule that
  chooses between them compares `p^dim` with the pool size, pricing a membership
  test and a rank test the same, and nothing had ever checked that. Forced onto
  each route in turn it is right on all four questions timed, including one where
  it sends a 225-element pool to the subspace walk and the walk wins, so **the
  cost-weighted rule this was meant to become is not made**:
  [`exhaustive_search/which-leaf-route-is-cheaper.md`](exhaustive_search/which-leaf-route-is-cheaper.md).

- **A check that the front page is the results files**,
  [`reproduce/front_page.py`](reproduce/front_page.py), run in CI. `index.html`
  cited `descent_search/results.json` for two charts and a table and read it
  never; the charts fetch it now, and the table, which stays hand-typeset so the
  evidence survives JavaScript being off, is refused by CI when it drifts.

### Changed

- **BREAKING: `sparsify-operator --exact` is gone**, because the method it
  selected is the default now. `--operations` opts into the greedy by rescaling,
  which minimises `nnz + nns` rather than `nnz`; `--simplex` answers by linear
  programming; `--emit PATH` writes the answer as SMS, the way up the file came
  in. A script passing `--exact` is refused rather than silently ignored, which
  `matrix_sparsification/tests/check_every_route_answers.sh` asserts.

- **The default run costs about a third of a second rather than about 500 s**,
  because it no longer runs five methods to report a comparison. Three of them
  reached the same counts 88x to 343x more slowly and moved to the
  archive branch (since 2026-08-23 `rejected-experiments`,
  `retired/dominated_sparsifiers/`) with their tests and measurements:
  `matrix_sparsification/dominated.md` says what went and where to find it.
  Nothing was deleted, and two of the three are `[beniamini2020]`'s own
  Algorithms 3 and 4.

- `--max-memory` now prices the *walk* rather than an allocation. The scan
  allocates almost nothing, so the budget had stopped reaching the command; it is
  the column supports the scan may visit that runs away. `4x4x4_49_156_L` is
  refused in milliseconds at 1.4 PiB where it used to run for thirty minutes and
  say nothing.

- **The SMS reader refuses what LinBox would read differently**, which is three
  corrections read off his sources rather than off our notes about them. Its
  comment said the format does not fix how many triples go on a line; it does,
  because LinBox reads a value with Givaro's line-greedy rational operator, so
  `1 1 5 3 2 7` is one entry of 5327 and `sms2pretty` prints exactly that with
  no warning — accepting it meant holding a different matrix from his out of the
  same bytes. Entries are not always rationals: three of his 153 carry
  polynomials in an indeterminate, now refused by name. And the type letter
  carries nothing, since `PMchecker` reads every operator over the rationals and
  takes the field from `-q`, so `read_sms` gained the modular overload that
  `write_sms(ostream, ModularMatrix)` never had a partner for. The field-by-field
  comparison, with the file and line on both sides, is
  [`formats/interchange/where-the-conventions-differ.md`](formats/interchange/where-the-conventions-differ.md);
  149 of his 153 matrices round-trip through the two halves and print
  identically under his own reader.

- **Twelve command-line tools, audited down from fourteen** and back to thirteen
  with `operators-to-tensor` above, with the one
  question each answers that no other does in
  [`OPTIONS/one-question-per-command.md`](OPTIONS/one-question-per-command.md).
  It was fourteen and not thirteen because `list-solvers` is the one command
  whose source is not a `*_main.cpp`, so every list built from that pattern had
  missed it. Two left, neither by merging into another tool's flag surface:
  `list-solvers` became **`curve-bounds --solvers`**, beside the `--route` its
  answer is only ever read to decide and the `--table` that already prints and
  stops there; and `price-canonical-route` moved out of `commands/` to be built
  beside the predicate it prices, as an instrument rather than a tool, because
  what it prints is nanoseconds and [`MEASURING.md`](MEASURING.md)'s line is that
  counts reproduce anywhere and timings do not.

  **Four merges were refused and the audit says what each would have cost**, the
  rule being that a command carrying two mutually exclusive flag sets is worse
  than two commands: the three deciders would have carried eight tree-only flags
  against thirteen solver-only ones and two meanings of `--max-memory`; the
  descent and the incumbent search collide on no name but leave four flags dead
  on half the command; `walk-scheme` collides on `--steps` and on `--from`; and
  the two in `oracle_guided_search/` share a directory rather than a question.
  A fifth was refused when the thirteenth arrived: `operators-to-tensor` into
  `make-tensor`, on the build order rather than on the flags, since building a
  map from its definition sits below the search and rebuilding one from a
  decomposition sits above it.

  The retired `list-solvers` still builds, no longer links `integer_programme`,
  and prints the line to type before leaving as 2. Deleting it would have been
  quieter and worse: `command not found` names no replacement, and a stub
  printing an empty ranking with 0 would read as a machine with no backends.

- **The console's tool list is checked against the build rather than counted.**
  `lower-the-bound` shipped and never reached `web_interface/catalogue.py`, and
  `len(tools) == 12` stayed true throughout, because it counted instead of
  corresponding. Both directions are now asserted: every binary the catalogue
  names exists, and every command the build produces is either offered or on a
  list of names deliberately not offered, each with its reason. Nine files that
  said "the twelve" where they meant "the tools" stopped naming a number.

- **Two stale rows in
  [`OPTIONS/one-idea-several-spellings.md`](OPTIONS/one-idea-several-spellings.md)**,
  found by running each enum-like flag against a fixture rather than reading the
  page. `--leaf-route` and `--anchor` were listed as the two bad patterns and
  both had been fixed in `decide_rank_main.cpp` without the page being sent back
  to check; `--device`, which is right, had never reached it. What is actually
  left is `--from` on `lower-the-bound`, which reports a bad value as an
  unrecognised flag, `--backend`, which accepts anything silently, and three that
  reprint a usage block. Ten such flags, four behaviours, where the page said
  nine and five.

- **`f2_5x5` is settled at 13 inside this repository**, and was
  `13 ≤ rank ≤ 14` before. The exhaustive search refutes 12 in 146 402 553 nodes
  and `lower-the-bound` exhibits 13 in 80, so `[bdez2012]`'s 9.65·10⁹ tests
  agree with the answer rather than being what it rests on.
  [`the-research-front/where-we-stand.md`](the-research-front/where-we-stand.md) and
  [`descent_search/known_ranks.md`](descent_search/known_ranks.md) both said the
  older bracket.

- **`--orbit-test full|generators` is documented**, which it was not anywhere
  outside the binary that parses it. It is in
  [`OPTIONS/searching-for-rank.md`](OPTIONS/searching-for-rank.md) with the
  duplication its cheap arm leaves, in
  [`how-the-search-works/parameters.md`](how-the-search-works/parameters.md)
  beside the parameter it moves, and in the browser console's catalogue. The same
  pass added `--anchor` to the parameters page, which had never been there
  either, and `walk-scheme --steps` as the accepted older spelling of `--flips`.

- **Two pages that are about using the twelve rather than about one of them.**
  [`OPTIONS/common-recipes.md`](OPTIONS/common-recipes.md) is the line people
  actually type, one per question, each running against a shipped fixture; and
  [`OPTIONS/one-idea-several-spellings.md`](OPTIONS/one-idea-several-spellings.md)
  is where the twelve disagree with each other — five names for "which machine
  answers", `--node-limit` for four different budgets with defaults 25x apart,
  `--max-memory` in bytes on three tools and megabytes on two, and **five
  different behaviours for a bad enum value across the nine flags that take
  one**. `--orbit-test bogus` names the word and the two right answers;
  `--leaf-route bogus`, four lines away in the same file, says `unrecognised
  option: --leaf-route`; `--anchor` and `--backend` accept anything in silence.
  Nothing was renamed and no behaviour moved: a flag that has been published
  keeps its name, and what was missing was the page saying so.

- **The console teaches the command instead of only running it.** Six worked
  examples in [`web_interface/worked_examples.py`](web_interface/worked_examples.py)
  fill the map, the tool and the flags and stop there, four of them the same map
  one flag apart so that **found**, **proved** and **gave up** are three presses;
  the plan each tool prints — its pool, its leaf, its device, its quotient — is
  lifted out of the stream and put above the verdict by
  [`web_interface/plan_lines.py`](web_interface/plan_lines.py), copied and never
  reworded; and every command shown carries a button that copies it. Each example
  is *run* by the checks rather than read, and the promoted lines are asserted to
  still be the characters the tools print.

- **The canonical route stopped scanning the pool once per candidate child, and
  stopped asking the parent test for a minimum it never wanted.** Measured at
  five shapes, seven to fourteen whole pool scans a node were 98% of a canonical
  node at `<2,3,3>`, against 1.0% in the setwise stabiliser and 1.1% in the
  canonical image — the two operations the factored presentation and Linton's
  algorithm exist to make cheap were cheap, and the cost was somewhere else.
  [`pool_cosets.h`](oracle_guided_search/pool_cosets.h) gets every child's
  content from one pass a node by grouping pool residues into cosets of the
  current span, and the parent test now exits at the first candidate parent that
  beats its own rather than naming all of them. `<2,3,3>` at 8 went from **98.0 s
  to 11.4 s**, `<2,2,4>` at 10 from 0.500 s to 0.144 s, and `<3,3,3>` at 10 from
  1.33x faster than the plain route to **2.07x**. Every node count at every shape
  and level is unchanged, which is how it is known that only the clock moved.

- **A budget that ran out no longer shares a colour with a refutation.** The
  console's cards were worded apart and painted alike, so `outcome.py` now
  carries `decides` — **found**, **proved** or **nothing proved** — one branch on
  the exit code, taken before a tool's own badge is applied and never from it.
  Only exit 1 reaches `proved`; a budget, a stop, the wall clock, a crash and a
  status with no name all land in `nothing proved`. It is `decide-rank`'s own
  vocabulary, which prints `FOUND`, `NO` and `GAVE UP`.

- **Three things `web_interface/not-production-ready-yet.md` recorded are
  closed**, and the page now says what closing each one did not reach. A run that
  starts a solver warns *before* it starts, naming the flag and the two numbers,
  where it would leave that solver holding a core after a stop; `serve.py` counts
  and sizes the runs it has accumulated instead of letting them grow unseen; and
  the fixture menu greys the maps a tool cannot read, from the tool's declared
  input rather than from any opinion about the file. The console's catalogue also
  stopped telling four tools that their `--max-memory` came from
  `sat_memory_megabytes`, which reaches only the two that call a solver.

- **Every stale leaf timing quoted outside the leaf's own directory is marked as
  stale rather than replaced.** `OPTIONS/searching-for-rank.md` quoted 129.1 ns a
  scanned element and 78 ns a walked one to convert `--leaf-limit` into seconds,
  and four ratios between the routes; `how-the-search-works/README.md` quoted 9.2
  minutes of one core for a `⟨4,4,4⟩` leaf, which was those same 129.1 ns times
  4 294 836 225. All were taken against the leaf as it stood before 2026-08-20.
  None has a measured replacement here and none is invented: what each was
  measured against is now written beside it, as
  [`positioning/hardware-and-parallelism.md`](positioning/hardware-and-parallelism.md)
  already does. The `-s` and `--general-leaf` rows moved to the node counts and
  the upper bounds that survive the change, since a node count is not a property
  of the leaf.

- **The front page describes the tool as it now is.** It said the rank of
  `⟨2,2,2⟩` was "decided in half a second", which was a wall clock from before
  the leaf moved twice, and it priced the orbit quotient at a wall-clock 28x from
  the same era: both are now the counts, which reproduce anywhere. The leaf's two
  rewrites and the browser console — neither of which the page mentioned — are on
  it, and it is still under its own length.

- **The GF(2) leaf's pool scan carries a residual instead of reducing every
  element.** Reduction modulo a subspace is linear, so `reduce(x ^ d)` is
  `reduce(x) ^ reduce(d)`; the pool is the outer-product grid, so for a fixed
  left mask two elements whose right masks differ in one bit differ by one of
  only `columns` patterns. Reducing those once per left leaves one exclusive-or
  and one zero test an element, the element is never formed, and the dimension
  leaves the inner loop entirely. Survivors are collected in Gray order, sorted
  per left and only then offered to the greedy, so the answer is bit-identical
  rather than merely equivalent — the same argument
  [`gpu_leaf/why-the-answer-is-the-same.md`](gpu_leaf/why-the-answer-is-the-same.md)
  already makes for the kernel. The vector lists are now built whether or not the
  packed table fits, since they cost 2 KB at 9x9 and were the only thing keeping
  this off the shapes anyone runs.

- **Two published figures were withdrawn as artefacts rather than measurements.**
  `canonical-augmentation.md` read a two-point line as a 0.196 s presentation fee
  when the two points sit at different targets, where a node costs seventeen
  times more; the presentation measures 0.013 s. And
  `comparing-against-the-baseline.md` published the baseline's deepest `work`
  level as if it were the array's sum, making the agreement look like 0.8% when
  it is 0.106%. Both corrections moved this repository's own numbers, not
  somebody else's.

- **The GPU harness is compiled on every build and linked on none.** It had not
  compiled since `Gf2Leaf` gained a `field` argument, on any machine, with the
  suite green throughout, because the directory was only read where the CUDA
  toolkit was found and no machine that builds this has `nvcc` on its PATH. A
  directory nothing compiles is a directory that drifts, and its numbers were
  being quoted meanwhile.

- **The general-field leaf decides rank one without computing a rank**, and its
  per-element path now allocates nothing at all. `linear_algebra::is_rank_one` in
  [`linear_algebra/measures.h`](linear_algebra/measures.h) takes a pointer and a
  shape rather than a `Matrix`, finds the first nonzero row, and cross-multiplies
  every later row against it — `a[p]·b[j] = b[p]·a[j]`, which is "is a multiple
  of" with the division cleared, so no modular inverse is taken and the answer
  returns at the first entry that disagrees. Against
  [`rank`](linear_algebra/measures.h), which is `O(r·d·c)` and builds a
  `SpanBasis`, copies every row into it and runs the elimination to the last row
  of a question settled at the second, this is `O(r·c)` in `Θ(1)` space.
  `keep_if_rank_one` in
  [`exhaustive_search/subspace_walk.cpp`](exhaustive_search/subspace_walk.cpp)
  now tests the combination buffer the walk already carries and forms a `Matrix`
  only for an element it keeps, which almost none are. No figure is claimed for
  it here; the timing is taken separately, under
  [`MEASURING.md`](MEASURING.md).

- **Same verdicts, same counts.** `is_rank_one` is `rank == 1` and is held
  against it over every 2x2, 2x3 and 3x2 matrix over GF(2), GF(3) and GF(5),
  exhaustively, plus a sample at 3x3 over GF(5), in
  [`exhaustive_search/tests/test_rank_one_predicate.cpp`](exhaustive_search/tests/test_rank_one_predicate.cpp).
  It is the predicate
  [`gf2_is_rank_one`](linear_algebra/gf2_bits.h) has always applied over GF(2),
  where the only nonzero scalar is 1 and the cross-multiplication degenerates to
  a word comparison; the two must agree about what rank one is, because the
  search sends a leaf down one path or the other on the characteristic alone.

- **The general-field leaf walks its subspace in base-`p` reflected Gray code
  order**, so an element costs one row added or subtracted rather than a rebuild
  from its base-`p` digits: `O(width)` field additions and no multiplication, in
  place of `O(dim * width)` multiply-accumulates. The successor is Knuth's
  Algorithm H, `[knuth2011]` §7.2.1.1, which names the digit to move in constant
  time, in
  [`descent_search/reflected_gray_walk.h`](descent_search/reflected_gray_walk.h);
  the leaf itself moved into
  [`exhaustive_search/subspace_walk.h`](exhaustive_search/subspace_walk.h).
  Measured on one core at **2.52x** an element over GF(3) at 3x6 dimension 12,
  **1.74x** over GF(5) and **1.58x** over GF(7), and the `dim` term is gone
  rather than reduced: the rebuild rises 32 ns an element per dimension added
  and the walk does not move. GF(2) is untouched and keeps its own packed walk
  in [`gf2_leaf.h`](exhaustive_search/gf2_leaf.h).

  **The order changed, so the leaf can hand back a different rank-one basis.**
  Not a different verdict: the leaf answers whether `needed` independent
  rank-one maps exist, which no reordering moves, and nothing asserted anywhere
  pins a decomposition. The route it replaced is kept beside it and the two are
  run against each other on the same span at every odd characteristic the
  fixtures carry.

### Fixed

- Every `results.json` was publishing an absolute home path into a public
  repository, the same defect the 2026-08-18 restructure fixed and the line that
  generates them put back. Fixed at the source.

- `index.html`'s sparsification chart read a field that the removed methods
  produced, and would have drawn nothing the next time `results.json` was
  regenerated.

### Retracted

- **`4x4x4_49_156_L` was published as having a regular column matroid and does
  not.** The claim rested on 200 000 random basis determinants all in `{0, ±1}`,
  which is not evidence: only about 0.8% of random 16-subsets are bases at all.
  A decision procedure refuted it in under three milliseconds with a
  sixteen-column minor of determinant −2, recomputed here in exact arithmetic.
  What it cost is one sentence, that the linear programme's answer was minimal
  *by Tillmann's theorem*; it is an upper bound.
  `matrix_sparsification/what-was-corrected.md` has the rest.

## [0.2.0] - 2026-08-20

Two days on the leaf and on what runs it: the exhaustive search's leaf packed
into machine words, a candidate pool it no longer has to hold, one consumer
card measured against the path it would replace, and six fixtures aimed at
numbers somebody has published.

### Added

- **Six order-3 fixtures over GF(2), each aimed at a number somebody has
  published**: `gf32_multiplication` and `gf64_multiplication`, extending the
  field-extension family past the three this repository settles itself;
  `cyclic_f2_7`, whose rank `[wang2026]` closes at 13 from both sides; and
  `matmul_2x3x4`, `matmul_3x3x4` and `matmul_3x4x4`, the three small formats that
  paper leaves open. Shapes, naive costs, targets and the two moduli are in
  [`fixtures/published-targets.md`](fixtures/published-targets.md); the bounds are
  pinned on all six in `rank_metric_bound/tests/`, where they cost 4.1 s.

- **A GPU proof of concept for the leaf test**, `gpu_leaf/`, built only where
  `nvcc` is present and called by nothing else. One whole `<4,4,4>` leaf, all
  4 294 836 225 rank-one maps, in **1.019 s** against 1.12 hours for the same leaf
  on one core, with survivor sets compared map for map against the shipped leaf on
  thirteen questions.

- **A device ranking**, [`run_limits/device.h`](run_limits/device.h), in the shape
  `integer_programme/solver_chain.h` already uses for solvers: the order is fixed,
  the availability is not, and an absent backend is a state reported rather than an
  error found downstream. `decide-rank` prints which device would answer.
  **No GPU backend is compiled in**, and it says so.

- **A span held as its rank filtration**,
  [`descent_search/sorted_span.h`](descent_search/sorted_span.h). The leaf test and
  the minimum-weight cost both become dimensions of `R[1] ⊆ … ⊆ R[16]`, so a sort
  over `p^dim` becomes a counting pass over sixteen buckets and the state that
  survives is 24 KB at `<4,4,4>`.

- **The literature the leaf test sits in**, which was cited nowhere: the Segre and
  bounded-rank line, the three MinRank modellings with their Hilbert series, Yang's
  fixed-parameter result, CUDA finite-field elimination, and Heule's SAT benchmark
  suite.

### Changed

- **The leaf of the quotiented search now packs its bits and can be stopped.** It
  called `rank_one_basis_of` with two arguments defaulted, so every leaf there took
  the general Givaro path and no `--leaf-limit` reached it. **25.7x** on
  `matmul_3x3x3 --target 23 --node-limit 300`, at the same 300 nodes.

- **A pool element is formed in bits where no table holds it.** It was rebuilt as a
  Givaro matrix and packed back one field element at a time; the outer product now
  goes straight into words. **5.87x** on the same question with the pool addressed,
  and `gpu_leaf` measures 940.2 ns an element against 129.1 at `<4,4,4>`.

- **The quotient runs on a pool it cannot hold.** Its candidate list was a vector,
  its positions a table and its struck orbits a byte array, all sized by the pool
  and all now arithmetic or a predicate: **34.3 MB less** at `<3,3,3>` at identical
  node counts, and `--symmetry` is no longer refused where the grid does not fit.

- **`[wang2026]` is now cited at the arXiv version its numbers come from**, v10,
  because that preprint's table grew across ten revisions rather than being
  corrected in place, and three of the four bounds quoted from it are absent from
  v1. A citation that names a document not containing the number is the one
  failure [`references.md`](references.md) exists to prevent.

- **`rank(f2_5x5) >= 13` is this repository's own claim now**, not `[bdez2012]`'s.
  Refuting twelve products was recorded as never run and priced at seven hours; it
  is 146 402 553 nodes and ran, and the seven hours was an extrapolation from the
  general leaf's rate that the GF(2) leaf had already made stale.

### Removed

- **Two functions nothing called**, `lowest_rank_partition` and
  `rank_one_maps_within`, the first orphaned when three search routes were retired
  and the second never called at all.

## [0.1.0] - 2026-08-18

First public release: a C++20 library on Givaro for exact tensor and bilinear rank,
corrected, tested, and extended into four strands.

### Added

- **Exact linear algebra over `GF(p)` and over `Q`**, templated on the field so
  one implementation serves every strand: dense matrices, a basis in reduced row
  echelon form, exact solve and inverse, rank-one decomposition, the three
  flattenings of a tensor and the rank lower bounds built on them. Nothing is a
  float anywhere.
- **The descent heuristic**, three steps that state separately what each
  guarantees: a matroid greedy, optimal for the basis it picks, then two
  first-improvement passes that guarantee nothing. It recovers the encoding
  operators ⟨L, R, P⟩ from its own answer and writes them out.
- **The complete exhaustive search**, deciding whether a map has an algorithm
  with exactly `k` products by enumerating subspaces rather than subsets. A
  refutation that ran to exhaustion is a lower bound; reaching the node limit is
  a third verdict and is reported as itself.
- **A walk on the flip graph**, moving a working scheme sideways instead of
  building one, which gives upper bounds only.
- **Symmetry and orbit reduction**, quotienting all three searches by the group
  that fixes the target subspace, and orbit cubes for a solver to split on.
- **SAT and SMT encodings of the rank question**: Booleans and parity over
  `GF(2)`, one-hot with the field tables over `GF(p)`, and the equations handed
  straight to cvc5's theory of prime fields. Solvers are run as programs and
  never linked. A refutation can be written as a DRAT proof and checked by
  `drat-trim`, and a proof asked of a solver that writes none is refused rather
  than dropped. Håstad's reduction runs the other way too, turning a 3SAT
  formula into a tensor.
- **Matrix sparsification**, minimising the additions the multiplication count
  never sees: a row-basis heuristic, the article's two exact oracles, a greedy
  scoring `nnz + nns`, and an exact determinant-polynomial test of whether a
  wanted pattern is reachable at all.
- **Bounds from algebraic curves**, the two steps of the Chudnovsky roadmap that
  are integer arithmetic, with the two needing Riemann-Roch spaces left out and
  said to be left out.
- **The integer programme layer** the curve strand hands its step 3 to: exact
  simplex and branch and bound, fixed-column MPS output, and a chain of outside
  solvers whose points are re-checked and whose "infeasible" is never believed.
- **The Kronecker canonical form of a two-slice tensor**, exactly and in
  polynomial time, with no candidate pool: the minimal indices from the ranks of
  one block system, the elementary divisors from a Smith diagonal over
  `GF(p)[x]` taken forwards and reversed, and three internal counts that must
  agree before anything is returned. It reports a **bound** rather than a rank,
  because Ja'Ja's formula is a theorem over an algebraically closed field and
  falls short over a small one: twelve pencils settled by exhaustion are
  tabulated, three of which it gets wrong.
- **The rank as a factorisation over the canonical basis**, `S = C A` with every
  row of `A` rank one, returned with the receipt that checks it in one matrix
  product without rerunning the search. Two routes, a materialised pool and a
  solver that forms none, required to agree on every fixture.
- **Twelve command-line tools**: `minimise-rank`, `decide-rank`, `walk-scheme`,
  `decide-rank-by-sat`, `list-solvers`, `deflate-strictly`, `enumerate-subspaces`,
  `decide-rank-by-pencil`, `factor-over-canonical-basis`, `curve-bounds`,
  `sparsify-operator` and `make-tensor`, sharing one argument grammar, one clock,
  one set of exit codes and one file of tunables.

### Removed

- **The fixed-`k` finder** and the descending sweep built on it, to the
  `rejected-experiments` branch. The finder rested on an assumed asymmetry
  between the cost of acceptance and refutation, quoted at two orders of
  magnitude; measured with matched flags it is about one, the original figures
  having compared instances encoded with and without symmetry breaking. It is
  dominated by the descent on every fixture. The sweep existed to hand
  `find_rank` a bracket, and once `[yang2025]`'s rank sums closed the
  floor-to-rank gap there was nothing left for a bracket to save: measured, it
  loses on all seven fixtures. No answer either gave was ever wrong. Both sets of
  numbers stay, because they are the evidence for the removal.

[0.4.1]: https://github.com/Tewf/tensor-rank-toolkit/releases/tag/v0.4.1
[0.4.0]: https://github.com/Tewf/tensor-rank-toolkit/releases/tag/v0.4.0
[0.3.0]: https://github.com/Tewf/tensor-rank-toolkit/releases/tag/v0.3.0
[0.2.0]: https://github.com/Tewf/tensor-rank-toolkit/releases/tag/v0.2.0
[0.1.0]: https://github.com/Tewf/tensor-rank-toolkit/releases/tag/v0.1.0
