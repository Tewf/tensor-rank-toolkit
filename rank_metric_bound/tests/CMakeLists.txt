# Two bounds, checked against every fixture: their values, their soundness, and
# the two orderings that are theorems rather than measurements. `formats` brings
# the tensor reader, `cli` the clock the printed cost column uses.
add_executable(test_rank_metric_bound test_rank_metric_bound.cpp)
target_link_libraries(test_rank_metric_bound PRIVATE rank_metric_bound cli formats
                                                     project_warnings testing)
add_test(NAME rank_metric_bound COMMAND test_rank_metric_bound ${PROJECT_SOURCE_DIR}/fixtures)
