# Two bounds, checked against every fixture: their values, their soundness, and
# the two orderings that are theorems rather than measurements. `formats` brings
# the tensor reader, `cli` the clock the printed cost column uses.
#
# Not labelled `slow`, and the number is here so that stays a decision. It costs
# 4.65 s, of which 4.06 s arrived with the six fixtures of
# `fixtures/published-targets.md` and 2.1 s of that is `matmul_3x4x4` alone: its
# longest axis is 16, so a rank table over that axis is 2^16 contractions, and
# the `rank_lower_bound` call this test compares against pays for two more. That
# is a table walk and not a search, it is nowhere near the critical path of a
# `ctest` run that takes 86 s, and `slow` here means a question that is
# exponential rather than one that is merely the largest.
add_executable(test_rank_metric_bound test_rank_metric_bound.cpp)
target_link_libraries(test_rank_metric_bound PRIVATE rank_metric_bound cli formats
                                                     project_warnings testing)
add_test(NAME rank_metric_bound COMMAND test_rank_metric_bound ${PROJECT_SOURCE_DIR}/fixtures)
