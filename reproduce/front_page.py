"""That the front page's numbers are the ones the results files hold.

`index.html` typesets its evidence tables by hand, so a reader with no
JavaScript still sees them, and that makes the page a second copy of
`descent_search/results.json` and of `matrix_sparsification/results.json`. The
page warns about a mismatch in the browser; this refuses one before it lands,
which is the half that runs in CI.

`measure.py --check` re-derives the results files from the built commands and
knows nothing about the page. Between them the chain is closed: code to file
here, file to page below.

The three tables are not the same kind of claim and the difference is worth
keeping. The descent table's file is re-derived on every push. The
sparsification table's `against_plinopt` block is carried, because its three
operators are PLinOpt's own and ship nowhere here, so nothing re-derives it and
`questions.CARRIED` says so on every run. Checking the page against it is still
the whole of the point: nine numbers that lived only in HTML now live in one
file, and the page is held to that file rather than to whoever typed it.

The exact-answer table is the one that had no check at all, and it cost two
days: `f2_5x5` was settled at 13 on 2026-08-21 and the page went on printing
`>= 13` and "finding a thirteen ... has not been run" while every other surface
in the repository said the rank. Nothing here disagreed, because nothing here
looked. It is read now, out of the three blocks that settle a map, and so is the
sentence under it.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
PAGE = ROOT / "index.html"
DESCENT = ROOT / "descent_search" / "results.json"
SPARSIFICATION = ROOT / "matrix_sparsification" / "results.json"

# One table row, split into the thing it names and the cells after it. The page
# marks each row with `data-fixture` or `data-operator` for exactly this reason:
# matching on position would pass whenever two rows were swapped.
ROW = re.compile(r'<tr data-fixture="([^"]+)">(.*?)</tr>', re.S)
OPERATOR_ROW = re.compile(r'<tr data-operator="([^"]+)">(.*?)</tr>', re.S)
MAP_ROW = re.compile(r'<tr data-map="([^"]+)">(.*?)</tr>', re.S)
CELL = re.compile(r"<td[^>]*>(.*?)</td>", re.S)
# The columns of the sparsification table, in the order it prints them. The
# results file holds a third, what a neighbouring tool reached on the same three
# operators; the page prints the operator as given and the proved minimum, so
# that column is recorded there and asserted nowhere here.
COLUMNS = ("as_given", "minimum")
# The exact-answer table prints one famous tensor beside the fixtures, and that
# block names its rows in prose where the others name them by file. The mapping
# has to live somewhere, and it lives here for the same reason
# `questions.FAMOUS_FIXTURES` exists.
FAMOUS = {"<2,2,2> matrix multiplication": "matmul_2x2x2"}
# An inequality, in either of the two spellings the page has room for.
BOUND = re.compile(r"&[gl][te];|[<>]=|[≤≥]")


def numbers_in(cell):
    """The digits of one cell, with markup and entities dropped."""
    return re.sub(r"<[^>]+>|&[a-z]+;", " ", cell).strip()


def descent_disagreements(page):
    """Every row of the descent table that the results file does not support."""
    published = {row["name"]: row for row in json.loads(DESCENT.read_text())["fixtures"]}
    found = []

    for name, body in ROW.findall(page):
        found.append(name)
        if name not in published:
            yield f"{name}: the page has a row for it and {DESCENT.relative_to(ROOT)} does not"
            continue
        cells = [numbers_in(cell) for cell in CELL.findall(body)]
        expected = published[name]
        if int(cells[2]) != expected["naive"]:
            yield f"{name}: naive is {cells[2]} on the page and {expected['naive']} in the file"
        reached = expected["step_3"]["multiplications"]
        if int(cells[3]) != reached:
            yield f"{name}: reached is {cells[3]} on the page and {reached} in the file"
        seconds = expected["step_3"]["seconds"]
        if f"{float(cells[4].split()[0]):.2f}" != f"{seconds:.2f}":
            yield f"{name}: time is {cells[4]} on the page and {seconds} s in the file"

    for name in published:
        if name not in found:
            yield f"{name}: {DESCENT.relative_to(ROOT)} publishes it and the page has no row for it"


def settled_ranks():
    """The `bounded` maps this repository has since closed, and at what rank.

    A `bounded` row publishes refutations, not a rank, which is the honest shape
    for a map only bounded. `settled` appears on it once two searches have met:
    the exhaustion refuses everything below and `lower-the-bound` exhibits the
    number itself, and neither is the answer alone. It is carried rather than
    re-derived, at the price `questions.CARRIED` states, so this holds the page
    to the field and `measure.py --check` holds the field to nothing. That is
    said here rather than left to be assumed.
    """
    bounded = json.loads(DESCENT.read_text())["exact_search"]["bounded"]
    return {row["map"]: row["settled"] for row in bounded if "settled" in row}


def exact_answers():
    """(naive, exact answer) per map, from the three blocks that publish one.

    Three kinds of question settle a map and the page prints them in one table:
    `decided` is one `decide-rank` finishes by itself, `famous_tensors` is one
    the complexity literature argues about, and `bounded` is one closed from
    both ends by two searches rather than by either.
    """
    document = json.loads(DESCENT.read_text())
    naive = {row["name"]: row["naive"] for row in document["fixtures"]}
    answers = dict(settled_ranks())

    for row in document["exact_search"]["decided"]:
        naive[row["map"]] = row["naive"]
        answers[row["map"]] = row["exact"]
    for row in document["famous_tensors"]["runs"]:
        name = FAMOUS.get(row["tensor"])
        if name is not None:
            naive[name] = row["naive"]
            answers[name] = row["exact"]["found"]
    return naive, answers


def exact_answer_disagreements(page):
    """Every row of the exact-answer table the results file does not support.

    One direction only, unlike the two tables around it: the page prints a
    selection and the file publishes more than it, `f2_2x2`'s 2x3 sibling among
    them, so a map the page leaves out is an editorial choice rather than drift.
    A map it prints has to be the file's, in both columns.
    """
    naive, answers = exact_answers()

    for name, body in MAP_ROW.findall(page):
        if name not in answers:
            yield f"{name}: the page prints an exact answer and {DESCENT.relative_to(ROOT)} publishes none"
            continue
        raw = CELL.findall(body)
        cells = [numbers_in(cell) for cell in raw]
        if int(cells[1]) != naive[name]:
            yield f"{name}: naive is {cells[1]} on the page and {naive[name]} in the file"
        if int(cells[2]) != answers[name]:
            yield (f"{name}: the exact answer is {cells[2]} on the page and "
                   f"{answers[name]} in the file")
        # `&ge; 13` reads as 13 once the entities are dropped, which is how the
        # bound this table used to print would have passed the line above while
        # saying something else entirely. A column headed "Exact answer" carries
        # a number, so an inequality in one is a failure whatever it evaluates to.
        if BOUND.search(raw[2]):
            yield f"{name}: the exact answer is qualified on the page and the column is not"

    # The paragraph under the table is the same claim in words, and a paragraph
    # is as copied as a table cell. This is the half that was wrong: the row said
    # `>= 13` and the sentence under it said finding a thirteen "has not been
    # run", two days after `lower-the-bound` had run it.
    prose = re.sub(r"<[^>]+>", "", page)
    for name, settled in settled_ranks().items():
        if f"the rank is exactly {settled}" not in prose:
            yield (f"{name}: the paragraph under the table no longer says "
                   f"\"the rank is exactly {settled}\"")


def sparsification_disagreements(page):
    """Every cell of the Grey-221 table the results file does not support.

    The total row is checked against the column sums rather than against a
    fourth measurement, because it is not one: three operators are what was
    measured and the totals are arithmetic on them. A total that stops adding up
    is then a failure here instead of a number nobody re-added.
    """
    block = json.loads(SPARSIFICATION.read_text())["against_plinopt"]
    published = {row["operator"]: row for row in block["operators"]}
    totals = {column: sum(row[column] for row in block["operators"]) for column in COLUMNS}
    found = []

    for name, body in OPERATOR_ROW.findall(page):
        found.append(name)
        expected = totals if name == "total" else published.get(name)
        if expected is None:
            yield f"{name}: the page has a row for it and {SPARSIFICATION.relative_to(ROOT)} does not"
            continue
        cells = [numbers_in(cell) for cell in CELL.findall(body)][1:]
        if len(cells) != len(COLUMNS):
            yield (f"{name}: {len(cells)} columns after the operator, and the page "
                   f"prints {len(COLUMNS)}")
            continue
        for column, cell in zip(COLUMNS, cells):
            if int(cell) != expected[column]:
                yield (f"{name}: {column} is {cell} on the page and "
                       f"{expected[column]} in the file")

    for name in [*published, "total"]:
        if name not in found:
            yield f"{name}: {SPARSIFICATION.relative_to(ROOT)} publishes it and the page has no row for it"

    # The sentence above the table quotes the same totals, and a paragraph is as
    # copied as a table cell. Read with the markup dropped, so rewording around
    # the numbers is not a failure and changing one of them is.
    prose = re.sub(r"<[^>]+>", "", page)
    expected = f"{totals['as_given']} nonzeros to {totals['minimum']}"
    if expected not in prose:
        yield f"the paragraph above the table no longer says \"{expected}\""


def disagreements():
    """Every row of the page that its results file does not support."""
    page = PAGE.read_text()
    yield from descent_disagreements(page)
    yield from exact_answer_disagreements(page)
    yield from sparsification_disagreements(page)


def main():
    failures = list(disagreements())
    for failure in failures:
        print(f"index.html: {failure}")
    if failures:
        print(f"# {len(failures)} disagreements. The results file is the source.")
        return 1
    print(f"# index.html agrees with {DESCENT.relative_to(ROOT)} and "
          f"{SPARSIFICATION.relative_to(ROOT)} on every row.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
