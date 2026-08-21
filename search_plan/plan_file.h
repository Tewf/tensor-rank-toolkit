#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "search_plan.h"

/// A plan written down, so a run somewhere else makes the same seven choices.
///
/// **Why a file at all.** The rules read this machine: how much memory it will
/// lend, whether a card answered, where the launch floor is. A cloud run reading
/// its own machine reaches its own answers, which is right for a fresh run and
/// wrong for reproducing one. `--plan-out` writes what was chosen and
/// `--plan-in` replays it, so the two runs differ in their hardware and not in
/// their decisions.
///
/// **`name value`, one per line, `#` starts a comment.** That is
/// [`../cli/tunables.h`](../cli/tunables.h)'s format without the `=`, for the
/// reason that file gives: this repository has no JSON reader, a plan is seven
/// flat values, and a format a person can diff is worth more here than one a
/// library can nest. Every field is always written, so the file documents the
/// format by existing.
///
/// **A name that is not a field is refused rather than ignored**, and so is a
/// value that is not one of the words the field takes. A plan that was quietly
/// half-read would reproduce a run that never happened, which is the one failure
/// a plan file exists to prevent.
namespace bilinear_rank {

void write_plan(std::ostream& out, const SearchPlan& plan);
void write_plan_file(const std::string& path, const SearchPlan& plan);

/// The plan a file describes. Its three reasons say where it came from, since a
/// file carries decisions and not the arithmetic that reached them.
SearchPlan read_plan(std::istream& text, const std::string& path);
SearchPlan read_plan_file(const std::string& path);

}  // namespace bilinear_rank
