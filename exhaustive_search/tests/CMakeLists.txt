# A decision is only worth having if the algorithm it claims can be rebuilt, so
# the test recovers it, which is descent_search's side of the pipeline.
add_executable(test_exhaustive_search test_exhaustive_search.cpp)
target_link_libraries(test_exhaustive_search PRIVATE exhaustive_search descent_search
                                                     formats testing project_warnings)
add_test(NAME exhaustive_search COMMAND test_exhaustive_search ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(exhaustive_search PROPERTIES TIMEOUT 900)

# The other half of `12 <= rank(F2 5x5)`: 459 239 nodes, about 77 seconds. The
# 9 and 10 exclusions cost nothing and stay in the test above.
add_test(NAME f2_5x5_has_no_11_products
         COMMAND test_exhaustive_search ${PROJECT_SOURCE_DIR}/fixtures 11)
set_tests_properties(f2_5x5_has_no_11_products PROPERTIES LABELS slow TIMEOUT 900)

# Why the branch and bound has no per-node bound: it would be the free test.
# The header says what degrades and why; this pins the arithmetic so the day a
# bound stops reading the least rank in the space, the failure is the news.
add_executable(test_bound_at_a_node test_bound_at_a_node.cpp)
target_link_libraries(test_bound_at_a_node PRIVATE exhaustive_search descent_search
                                                   formats testing project_warnings)
add_test(NAME bound_at_a_node COMMAND test_bound_at_a_node ${PROJECT_SOURCE_DIR}/fixtures)
