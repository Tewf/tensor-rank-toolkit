# Mitigations not built, with reasons

- **Incremental solving through IPASIR**, expected the largest gain. Not built.
  `libcadical-dev 1.7.4-1` and `libcryptominisat5-dev` are both in the Ubuntu archive
  and neither is installed. A `pkg_check_modules` line would break the other two
  branches building in this repository until they install it too, so it belongs behind
  an optional `find_package(... QUIET)` seam decided in one place. It also needs hit
  rate measured, not only speed: an incremental solver is often weaker per call than a
  fresh one, so it can lose while looking faster.
- **Yang's rank-table pruners.** Not duplicated on purpose: `yang-search` is building
  `ranksum` and the shared `ranks[v]` table now. `FinderSettings::floor` is the seam.
- **Memoisation.** No question is asked twice at this depth, so it has nothing to
  return. It earns its place only once a sweep re-asks a tightened question.

None of this is needed to reach the mitigations that are built: the run in
[`README.md`](README.md) shows the tree route directly, and
[`the-mitigations.md`](the-mitigations.md) prices the cumulative gain without any
of the three above at roughly **700x**.
