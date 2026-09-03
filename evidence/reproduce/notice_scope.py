#!/usr/bin/env python3
"""That NOTICE claims every directory the build adds, and counts what it credits.

NOTICE says its MIT list "is every directory CMakeLists.txt adds", and that a
folder missing from it is a folder whose licence the file does not state. That
sentence was a promise kept by hand, and it drifted twice: four directories
were missing until 2026-08-20, two more until 2026-09-03. This compares the
list against the build system it quotes, the way front_page.py compares the
front page against the results files.

Two checks, both exact:
  - every `add_subdirectory(dir)` in the top CMakeLists.txt appears in NOTICE
    (vendor/ enters as its basename: PermLib has its own section);
  - the number NOTICE writes for evidence/fixtures/plinopt/'s matrices is the number of
    `.sms` files sitting there.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent

NUMBER_WORDS = {
    "one": 1, "two": 2, "three": 3, "four": 4, "five": 5, "six": 6,
    "seven": 7, "eight": 8, "nine": 9, "ten": 10, "eleven": 11, "twelve": 12,
    "thirteen": 13, "fourteen": 14, "fifteen": 15, "sixteen": 16,
    "seventeen": 17, "eighteen": 18, "nineteen": 19, "twenty": 20,
}


def failures():
    notice = (ROOT / "NOTICE").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

    for directory in re.findall(r"^\s*add_subdirectory\(([^)\s]+)\)", cmake,
                                flags=re.MULTILINE):
        name = directory.split("/")[-1]
        if name.lower() not in notice.lower():
            yield (f"CMakeLists.txt adds {directory}/ and NOTICE never names "
                   f"it, so its licence is stated nowhere")

    counted = re.search(r"evidence/fixtures/plinopt/ is (\w+) `\.sms` matrices", notice)
    if counted is None:
        yield "NOTICE no longer counts evidence/fixtures/plinopt/'s matrices at all"
        return
    word = counted.group(1)
    if word not in NUMBER_WORDS:
        yield f"NOTICE counts evidence/fixtures/plinopt/ as '{word}', not a number word"
        return
    on_disk = len(list((ROOT / "evidence/fixtures" / "plinopt").glob("*.sms")))
    if NUMBER_WORDS[word] != on_disk:
        yield (f"NOTICE says {word} ({NUMBER_WORDS[word]}) `.sms` matrices; "
               f"evidence/fixtures/plinopt/ holds {on_disk}")


def main():
    found = list(failures())
    for failure in found:
        print(f"  FAIL  {failure}")
    if found:
        return 1
    print("NOTICE names every directory the build adds, and its count is the directory's")
    return 0


if __name__ == "__main__":
    sys.exit(main())
