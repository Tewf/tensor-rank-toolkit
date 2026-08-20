"""That the front page's numbers are the ones the results files hold.

`index.html` typesets its evidence table by hand, so a reader with no JavaScript
still sees it, and that makes the page a second copy of `descent_search/
results.json`. The page warns about a mismatch in the browser; this refuses one
before it lands, which is the half that runs in CI.

`measure.py --check` re-derives the results files from the built commands and
knows nothing about the page. Between them the chain is closed: code to file
here, file to page below.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
PAGE = ROOT / "index.html"
RESULTS = ROOT / "descent_search" / "results.json"

# One table row, split into the fixture it names and the cells after it. The
# page marks each row with `data-fixture` for exactly this reason: matching on
# position would pass whenever two rows were swapped.
ROW = re.compile(r'<tr data-fixture="([^"]+)">(.*?)</tr>', re.S)
CELL = re.compile(r"<td[^>]*>(.*?)</td>", re.S)


def numbers_in(cell):
    """The digits of one cell, with markup and entities dropped."""
    return re.sub(r"<[^>]+>|&[a-z]+;", " ", cell).strip()


def disagreements():
    """Every row of the page that the results file does not support."""
    page = PAGE.read_text()
    published = {row["name"]: row for row in json.loads(RESULTS.read_text())["fixtures"]}
    found = []

    for name, body in ROW.findall(page):
        found.append(name)
        if name not in published:
            yield f"{name}: the page has a row for it and {RESULTS.name} does not"
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
            yield f"{name}: {RESULTS.name} publishes it and the page has no row for it"


def main():
    failures = list(disagreements())
    for failure in failures:
        print(f"index.html: {failure}")
    if failures:
        print(f"# {len(failures)} disagreements. The results file is the source.")
        return 1
    print(f"# index.html agrees with {RESULTS.relative_to(ROOT)} on every row.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
