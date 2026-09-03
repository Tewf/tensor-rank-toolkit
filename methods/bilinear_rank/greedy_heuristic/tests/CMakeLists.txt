add_executable(test_pipeline test_pipeline.cpp)
target_link_libraries(test_pipeline PRIVATE greedy_heuristic exhaustive formats
                                            testing project_warnings)
add_test(NAME pipeline COMMAND test_pipeline ${PROJECT_SOURCE_DIR}/fixtures)

# Step 3 searches every rank-one map of the shape, so it is the expensive one.
# In Julia these took an hour and a half, or never finished; they are labelled
# so `ctest -LE slow` skips them, but they are cheap enough to keep in CI.
add_executable(test_step3 test_step3.cpp)
target_link_libraries(test_step3 PRIVATE greedy_heuristic formats testing project_warnings)

# f3_3x6 expects 10, which this library reaches where the published figure was
# never terminated, so the published 11 came from an abandoned run.
foreach(case "f2_5x5;14" "f2_3x8;15" "f2_4x7;16" "f3_3x6;10")
  list(GET case 0 case_name)
  list(GET case 1 case_expected)
  add_test(NAME step3_${case_name}
           COMMAND test_step3 ${PROJECT_SOURCE_DIR}/fixtures ${case_name} ${case_expected})
  set_tests_properties(step3_${case_name} PROPERTIES LABELS slow TIMEOUT 900)
endforeach()

add_executable(test_algorithm_recovery test_algorithm_recovery.cpp)
target_link_libraries(test_algorithm_recovery PRIVATE greedy_heuristic formats testing project_warnings)
add_test(NAME algorithm_recovery COMMAND test_algorithm_recovery ${PROJECT_SOURCE_DIR}/fixtures)

# The other direction of algorithm_recovery.h, and the only place the ⟨L,R,P⟩
# convention is checked against somebody else's rather than against itself: his
# published operators are made to rebuild a fixture written here from the
# definition of the map.
add_executable(test_operators_to_tensor test_operators_to_tensor.cpp)
target_link_libraries(test_operators_to_tensor PRIVATE greedy_heuristic map_construction formats
                                                       testing project_warnings)
add_test(NAME operators_to_tensor
         COMMAND test_operators_to_tensor ${PROJECT_SOURCE_DIR}/fixtures)

# And the command, because the test above reads the three files itself and so
# cannot see the order they are passed in, which is the whole of the interface.
add_test(NAME operators_to_tensor_command
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_operators_to_tensor.sh
                 $<TARGET_FILE:operators-to-tensor> ${PROJECT_SOURCE_DIR}/fixtures
                 $<TARGET_FILE:minimise-rank>)
set_tests_properties(operators_to_tensor_command PROPERTIES TIMEOUT 60)

add_executable(test_candidate_pool test_candidate_pool.cpp)
target_link_libraries(test_candidate_pool PRIVATE greedy_heuristic testing project_warnings)
add_test(NAME candidate_pool COMMAND test_candidate_pool)

# The theorems of correctness.md, executed. T5 in particular is a live check on
# the pruning: if a discarded candidate could ever have improved the result,
# this fails rather than returning a quietly worse answer.
add_executable(test_descent_guarantees test_descent_guarantees.cpp)
target_link_libraries(test_descent_guarantees PRIVATE greedy_heuristic orbit_reduction
                                                      formats testing project_warnings)
add_test(NAME descent_guarantees COMMAND test_descent_guarantees ${PROJECT_SOURCE_DIR}/fixtures)

# The filtration answers what the greedy answers, on every fixture and on spans
# the search grows rather than reads. A rewrite that is nearly right is the way
# four published step-1 numbers would move without anyone noticing.
add_executable(test_sorted_span test_sorted_span.cpp)
target_link_libraries(test_sorted_span PRIVATE greedy_heuristic formats testing project_warnings)
add_test(NAME sorted_span COMMAND test_sorted_span ${PROJECT_SOURCE_DIR}/fixtures)

# The order both spans here are walked in, asserted on its own. Neither property
# it rests on (every string once, one digit by one) shows up in a verdict, so a
# broken successor would come back as a wrong rank rather than as a failure
# anywhere else.
add_executable(test_reflected_gray_walk test_reflected_gray_walk.cpp)
target_link_libraries(test_reflected_gray_walk PRIVATE greedy_heuristic testing project_warnings)
add_test(NAME reflected_gray_walk COMMAND test_reflected_gray_walk)

# The Gray order the span is walked in against the index order it used to be
# walked in, entry for entry. Neither the carried index nor the incremental
# combination shows up in a verdict, so a drift in either returns a
# minimum-weight basis that is merely plausible.
add_executable(test_span_element_ranks test_span_element_ranks.cpp)
target_link_libraries(test_span_element_ranks PRIVATE greedy_heuristic formats testing
                                                      project_warnings)
add_test(NAME span_element_ranks COMMAND test_span_element_ranks ${PROJECT_SOURCE_DIR}/fixtures)

# The GF(2) representation against the general one, on the three calls step 1
# offers and on the primitive under them. A representation change that is nearly
# right is how every count in results.json would move at once, so the comparison
# is entry for entry rather than by size or by sum.
add_executable(test_gf2_span_walk test_gf2_span_walk.cpp)
target_link_libraries(test_gf2_span_walk PRIVATE greedy_heuristic formats testing project_warnings)
add_test(NAME gf2_span_walk COMMAND test_gf2_span_walk ${PROJECT_SOURCE_DIR}/fixtures)

# The five gates a card is offered a span through, on a machine with no card.
# The CUDA backend is the only one that exists, so before this every gate in
# `ranks_from_a_card` was dead code on any build without `nvcc`, CI included.
# The fake reads its answer out of the packed span alone, which is what makes
# this a check on the packing and the slice order rather than only on the branch.
add_executable(test_span_ranks_seam test_span_ranks_seam.cpp)
target_link_libraries(test_span_ranks_seam PRIVATE greedy_heuristic testing project_warnings)
add_test(NAME span_ranks_seam COMMAND test_span_ranks_seam)
