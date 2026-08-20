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

# The table route and the on-demand route must produce the same element. A
# disagreement is a wrong answer nothing downstream catches: both feed the same
# membership test, and a corrupted map simply fails to be in the span.
add_executable(test_packed_generation test_packed_generation.cpp)
target_link_libraries(test_packed_generation PRIVATE exhaustive_search descent_search
                                                     formats testing project_warnings)
add_test(NAME packed_generation COMMAND test_packed_generation ${PROJECT_SOURCE_DIR}/fixtures)

# The order the general leaf visits a subspace in, asserted on its own. Neither
# property it rests on — every string once, one digit by one — shows up in a
# verdict, so a broken successor would come back as a wrong rank rather than as
# a failure anywhere else.
add_executable(test_reflected_gray_walk test_reflected_gray_walk.cpp)
target_link_libraries(test_reflected_gray_walk PRIVATE exhaustive_search testing project_warnings)
add_test(NAME reflected_gray_walk COMMAND test_reflected_gray_walk)

# The GF(2) pool scan carries a reduction along a Gray code instead of asking
# the span about every element, and owes the direct scan the same maps in the
# same order rather than merely as many. `try_add` is order-dependent, so a
# survivor offered out of index order comes back as a different rank-one basis
# of the same space, which no count anywhere would catch.
add_executable(test_residual_scan test_residual_scan.cpp)
target_link_libraries(test_residual_scan PRIVATE exhaustive_search descent_search
                                                 formats testing project_warnings)
add_test(NAME residual_scan COMMAND test_residual_scan ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(residual_scan PROPERTIES TIMEOUT 900)

# And the leaf built on it, against the rebuild it replaced, on the same span
# over each of the three odd characteristics the fixtures carry. The Gray order
# changes which rank-one basis comes back and may not change how many, which is
# the leaf's whole verdict.
add_executable(test_gray_walk_leaf test_gray_walk_leaf.cpp)
target_link_libraries(test_gray_walk_leaf PRIVATE exhaustive_search descent_search
                                                  formats testing project_warnings)
add_test(NAME gray_walk_leaf COMMAND test_gray_walk_leaf ${PROJECT_SOURCE_DIR}/fixtures)
