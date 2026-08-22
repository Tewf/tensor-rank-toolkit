"""That a ratio typed into prose is the one the tests assert.

`front_page.py` closes the chain for `index.html`'s descent table, and
`measure.py --check` closes it for the results files. Neither looks at prose.
The oracle-guided speedup is quoted in seven documents in three directories,
none of which is generated, and it sat at 22 779x while the test beside it
asserted 1 890 601 and 83 nodes, whose quotient is 22 778.3. Nothing was wrong
with the measurement; the rounding went up from a number ending in .33.

A wrong digit next to a correct citation is worse than a missing one, because
the citation lends it credit. So the constants come from the test that asserts
them, the ratio is computed here, and the prose is searched for the result.

Add a claim by naming the two constants and the string the documents must
carry. It is cheap, and worth it for any number a reader would quote back.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TEST = ROOT / "oracle_guided_search" / "tests" / "test_canonical_augmentation.cpp"


def asserted(name):
    """The integer a check::equal line in the parent test pins, by its message."""
    pattern = re.compile(r'check::equal\("' + re.escape(name) + r'"[^;]*?,\s*(\d+)\s*\)')
    found = pattern.search(TEST.read_text())
    if not found:
        raise SystemExit(f"{TEST.name}: no check::equal named {name!r}. "
                         f"If the test was reworded, reword it here too.")
    return int(found.group(1))


def grouped(value):
    """22778 as the documents write it, with a thin space every three digits."""
    return f"{value:,}".replace(",", " ")


def claims():
    """(what it is, the string every document below must carry, the documents)."""
    plain = asserted("the plain route pays 1 890 601 nodes for those 36")
    canonical = asserted("and the canonical route 83 for the 1")
    ratio = grouped(round(plain / canonical))
    return [(
        f"isomorph-free speedup, {plain} / {canonical} nodes",
        ratio,
        ["README.md",
         "README.fr.md",
         "how-the-search-works/README.md",
         "OPTIONS/committing-to-candidates.md",
         "canonical_factorisation/canonical-augmentation.md",
         "oracle_guided_search/deduplication-cost.md",
         "oracle_guided_search/when-canonical-pays/why-nothing-consults-it.md"],
    )]


def flattened(text):
    """Prose as one line without emphasis, so rewrapping a paragraph is not a
    failure. Separators vary between documents, so they all read as one space."""
    return " ".join(re.sub(r"[  ,]", " ", text).replace("*", "").split())


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
        print(f"# {len(failures)} documents disagree with the test that asserts the "
              f"counts. The test is the source.")
        return 1
    print(f"# every quoted ratio matches {TEST.relative_to(ROOT)}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
