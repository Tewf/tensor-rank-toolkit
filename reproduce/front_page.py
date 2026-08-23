"""That the front page's numbers are the ones the results files hold.

`index.html` typesets its evidence tables by hand, so a reader with no
JavaScript still sees them, and that makes the page a second copy of
`descent_search/results.json` and of `matrix_sparsification/results.json`. The
page warns about a mismatch in the browser; this refuses one before it lands,
which is the half that runs in CI.

`measure.py --check` re-derives the results files from the built commands and
knows nothing about the page. Between them the chain is closed: code to file
here, file to page below.

The two tables are not the same kind of claim and the difference is worth
keeping. The descent table's file is re-derived on every push. The
sparsification table's `against_plinopt` block is carried, because its three
operators are PLinOpt's own and ship nowhere here, so nothing re-derives it and
`questions.CARRIED` says so on every run. Checking the page against it is still
the whole of the point: nine numbers that lived only in HTML now live in one
file, and the page is held to that file rather than to whoever typed it.
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
CELL = re.compile(r"<td[^>]*>(.*?)</td>", re.S)
# The three columns of the sparsification table, in the order it prints them.
COLUMNS = ("as_given", "plinopt", "minimum")


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
            yield f"{name}: {len(cells)} columns after the operator, and the file has 3"
            continue
        for column, cell in zip(COLUMNS, cells):
            if int(cell) != expected[column]:
                yield (f"{name}: {column} is {cell} on the page and "
                       f"{expected[column]} in the file")

    for name in [*published, "total"]:
        if name not in found:
            yield f"{name}: {SPARSIFICATION.relative_to(ROOT)} publishes it and the page has no row for it"

    # The sentence above the table quotes the same three totals, and a paragraph
    # is as copied as a table cell. Read with the markup dropped, so rewording
    # around the numbers is not a failure and changing one of them is.
    prose = re.sub(r"<[^>]+>", "", page)
    for expected in (f"{totals['as_given']} nonzeros to {totals['minimum']}",
                     f"against {totals['plinopt']}"):
        if expected not in prose:
            yield f"the paragraph above the table no longer says \"{expected}\""


def disagreements():
    """Every row of the page that its results file does not support."""
    page = PAGE.read_text()
    yield from descent_disagreements(page)
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
