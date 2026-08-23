"""That a ratio typed into prose is the one something else asserts.

`front_page.py` closes the chain for `index.html`'s two tables, and
`measure.py --check` closes it for the results files. Neither looks at prose, and
neither reaches the article, whose Table 2 could drift from `results.json`
silently. It is guarded here now: a LaTeX tabular carries no sentence to match,
but the row is a string like any other once the emphasis is dropped, and
`flattened` says how.
The oracle-guided speedup is quoted in eight documents in three directories,
none of which is generated, and it sat at 22 779x while the test beside it
asserted 1 890 601 and 83 nodes, whose quotient is 22 778.3. Nothing was wrong
with the measurement; the rounding went up from a number ending in .33.

A wrong digit next to a correct citation is worse than a missing one, because
the citation lends it credit. So the constants come from whatever asserts them,
the ratio is computed here, and the prose is searched for the result.

Two things assert a constant in this repository and both are read below. A test
asserts one directly, and `asserted` reads the integer out of its `check::equal`
line. A results file asserts one at second hand: `measure.py --check` re-derives
every count in the sections `questions.SECTIONS` names, on every push, so a
number in one of those is as pinned as a test's. `published` reads those. What
neither covers is a timing, which MEASURING.md says is asserted nowhere and by
nothing, so a wall-clock ratio like the canonical route's 11.8x has no source
here and is not claimed to have one.

Add a claim by naming where the constants come from and the string the documents
must carry. It is cheap, and worth it for any number a reader would quote back.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TEST = ROOT / "oracle_guided_search" / "tests" / "test_canonical_augmentation.cpp"
DESCENT = ROOT / "descent_search" / "results.json"
SPARSIFICATION = ROOT / "matrix_sparsification" / "results.json"


def asserted(name):
    """The integer a check::equal line in the parent test pins, by its message."""
    pattern = re.compile(r'check::equal\("' + re.escape(name) + r'"[^;]*?,\s*(\d+)\s*\)')
    found = pattern.search(TEST.read_text())
    if not found:
        raise SystemExit(f"{TEST.name}: no check::equal named {name!r}. "
                         f"If the test was reworded, reword it here too.")
    return int(found.group(1))


def published(path, section, named, field):
    """The number one row of a results file holds, by the section and the row.

    Only sections `measure.py --check` re-derives belong here. A figure it
    carries rather than asks is pinned by nothing, and quoting it as though a
    run agreed with it is the failure this file exists to stop.
    """
    for row in json.loads(path.read_text()).get(section, []):
        if named in row.values():
            if field not in row:
                raise SystemExit(f"{path.name}: {named!r} has no {field!r}.")
            return row[field]
    raise SystemExit(f"{path.name}: no {section} row named {named!r}. "
                     f"If the row was renamed, rename it here too.")


def grouped(value):
    """22778 as the documents write it, with a thin space every three digits."""
    return f"{value:,}".replace(",", " ")


def totals_of(block, column):
    """One column of a carried comparison, added up as the page adds it up."""
    return sum(row[column] for row in block["operators"])


def claims():
    """(what it is, the string every document below must carry, the documents)."""
    plain = asserted("the plain route pays 1 890 601 nodes for those 36")
    canonical = asserted("and the canonical route 83 for the 1")
    quotient = published(DESCENT, "quotient", "<2,2,2> refuting 6 products", "fewer_nodes_by")
    grey = json.loads(SPARSIFICATION.read_text())["published_scheme_operators"]
    as_given, minimum = (totals_of(grey, column)
                         for column in ("as_given", "minimum"))
    return [(
        f"isomorph-free speedup, {plain} / {canonical} nodes",
        grouped(round(plain / canonical)),
        ["README.md",
         "README.fr.md",
         "what-it-computes.md",
         "how-the-search-works/README.md",
         "OPTIONS/committing-to-candidates.md",
         "canonical_factorisation/canonical-augmentation.md",
         "oracle_guided_search/deduplication-cost.md",
         "oracle_guided_search/when-canonical-pays/why-nothing-consults-it.md"],
    ), (
        "what the orbit quotient removes refuting 6 products on <2,2,2>",
        f"{quotient}x",
        ["README.md",
         "what-it-computes.md",
         "how-the-search-works/what-to-wire.md",
         "OPTIONS/searching-for-rank.md",
         "orbit_reduction/README.md",
         "orbit_reduction/what-the-quotient-costs.md",
         "orbit_reduction/orbit_search.cpp"],
    ), (
        "the Grey-221 operators, as given and at the proved minimum",
        f"{as_given} nonzeros to {minimum}",
        ["README.md", "what-it-computes.md", "article/bilinear-rank.tex"],
    ), (
        "the same, in French",
        f"{as_given} non-nuls à {minimum}",
        ["README.fr.md"],
    ), (
        "the same two totals as the article's table row",
        f"& {as_given} & {minimum} &",
        ["article/bilinear-rank.tex"],
    )]


def flattened(text):
    """Prose as one line without emphasis, so rewrapping a paragraph is not a
    failure. Separators vary between documents, so they all read as one space,
    and the multiplication sign reads as the letter: a document writing 39.2×
    where another writes 39.2x is not making a different claim, and both
    spellings ship here.

    **Emphasis has three spellings across these documents and none of them is
    part of a claim**: `**` in Markdown, `$` around a number in LaTeX, and
    `\\mathbf{}` around one. Dropping all three is what lets one string cover
    the article and the Markdown that quotes it, and it is what makes the
    article's table reachable at all: a LaTeX table carries no prose sentence,
    so the row itself has to be the string, and `& 221 & 128 &` is that row once
    the money and the boldface are gone."""
    plain = text.replace("×", "x").replace("$", "")
    plain = re.sub(r"\\mathbf\{([^}]*)\}", r"\1", plain)
    return " ".join(re.sub(r"[ \xa0,]", " ", plain).replace("*", "").split())


def disagreements():
    for what, expected, documents in claims():
        for document in documents:
            path = ROOT / document
            if not path.exists():
                yield f"{document}: quoted for {what} and the file is gone"
            elif flattened(expected) not in flattened(path.read_text()):
                yield f"{document}: expected \"{expected}\" ({what})"


def main():
    failures = list(disagreements())
    for failure in failures:
        print(f"quoted number: {failure}")
    if failures:
        print(f"# {len(failures)} documents disagree with what asserts the counts. "
              f"The test and the results files are the source.")
        return 1
    print(f"# every quoted ratio matches {TEST.relative_to(ROOT)} or the results file "
          f"beside it.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
