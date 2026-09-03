# A decision is only worth having if the algorithm it claims can be rebuilt, so
# the test recovers it, which is greedy_heuristic's side of the pipeline.
add_executable(test_exhaustive_search test_exhaustive_search.cpp)
target_link_libraries(test_exhaustive_search PRIVATE exhaustive greedy_heuristic
                                                     formats testing project_warnings)
add_test(NAME exhaustive COMMAND test_exhaustive_search ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(exhaustive PROPERTIES TIMEOUT 900)

# The other half of `12 <= rank(F2 5x5)`: 459 239 nodes, about 77 seconds. The
# 9 and 10 exclusions cost nothing and stay in the test above.
add_test(NAME f2_5x5_has_no_11_products
         COMMAND test_exhaustive_search ${PROJECT_SOURCE_DIR}/fixtures 11)
set_tests_properties(f2_5x5_has_no_11_products PROPERTIES LABELS slow TIMEOUT 900)

# That a trace says what the search did and costs it nothing. Both searches are
# asked, because they are different functions and a trace that worked for one
# while silently writing nothing for the other is what happened while this was
# being built.
add_test(NAME trace_is_a_tree
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_trace_is_a_tree.sh
                 $<TARGET_FILE:decide-rank> ${PROJECT_SOURCE_DIR}/fixtures)

# Why the branch and bound has no per-node bound: it would be the free test.
# The header says what degrades and why; this pins the arithmetic so the day a
# bound stops reading the least rank in the space, the failure is the news.
add_executable(test_bound_at_a_node test_bound_at_a_node.cpp)
target_link_libraries(test_bound_at_a_node PRIVATE exhaustive greedy_heuristic
                                                   formats testing project_warnings)
add_test(NAME bound_at_a_node COMMAND test_bound_at_a_node ${PROJECT_SOURCE_DIR}/fixtures)

# The table route and the on-demand route must produce the same element. A
# disagreement is a wrong answer nothing downstream catches: both feed the same
# membership test, and a corrupted map simply fails to be in the span.
add_executable(test_packed_generation test_packed_generation.cpp)
target_link_libraries(test_packed_generation PRIVATE exhaustive greedy_heuristic
                                                     formats testing project_warnings)
add_test(NAME packed_generation COMMAND test_packed_generation ${PROJECT_SOURCE_DIR}/fixtures)


# The GF(2) pool scan carries a reduction along a Gray code instead of asking
# the span about every element, and owes the direct scan the same maps in the
# same order rather than merely as many. `try_add` is order-dependent, so a
# survivor offered out of index order comes back as a different rank-one basis
# of the same space, which no count anywhere would catch.
add_executable(test_residual_scan test_residual_scan.cpp)
target_link_libraries(test_residual_scan PRIVATE exhaustive greedy_heuristic
                                                 formats testing project_warnings)
add_test(NAME residual_scan COMMAND test_residual_scan ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(residual_scan PROPERTIES TIMEOUT 900)

# The GF(2) subspace walk carries one combination along a Gray code instead of
# rebuilding each element from the digits of its index, and owes the rebuild the
# same maps in the same order rather than merely as many. Gray order is not index
# order, so the batches it is cut into have to be contiguous, aligned and drained
# upward; a boundary in the wrong place hands the greedy a different rank-one
# basis of the same space, which no count anywhere would catch.
add_executable(test_gf2_subspace_walk test_gf2_subspace_walk.cpp)
target_link_libraries(test_gf2_subspace_walk PRIVATE exhaustive greedy_heuristic
                                                     formats testing project_warnings)
add_test(NAME gf2_subspace_walk COMMAND test_gf2_subspace_walk ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(gf2_subspace_walk PROPERTIES TIMEOUT 900)

# And the leaf built on it, against the rebuild it replaced, on the same span
# over each of the three odd characteristics the fixtures carry. The Gray order
# changes which rank-one basis comes back and may not change how many, which is
# the leaf's whole verdict.
add_executable(test_gray_walk_leaf test_gray_walk_leaf.cpp)
target_link_libraries(test_gray_walk_leaf PRIVATE exhaustive greedy_heuristic
                                                  formats testing project_warnings)
add_test(NAME gray_walk_leaf COMMAND test_gray_walk_leaf ${PROJECT_SOURCE_DIR}/fixtures)

# And the one test the leaf now rests on: `is_rank_one` is the only thing that
# says yes to an element, so it is held against the elimination it replaced over
# every matrix of the shapes small enough to enumerate. It lives here rather than
# beside `measures.h` because this is the caller whose answers it decides.
add_executable(test_rank_one_predicate test_rank_one_predicate.cpp)
target_link_libraries(test_rank_one_predicate PRIVATE linear_algebra testing project_warnings)
add_test(NAME rank_one_predicate COMMAND test_rank_one_predicate)

# Asked of the built command rather than of the function, because the thing being
# asserted is a routing decision `main` makes: which of two modules answers.
add_test(NAME pencil_shortcut
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_pencil_shortcut.sh
                 $<TARGET_FILE:decide-rank> ${PROJECT_SOURCE_DIR}/fixtures)
