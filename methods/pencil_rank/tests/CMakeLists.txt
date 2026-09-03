add_executable(test_polynomial test_polynomial.cpp)
target_link_libraries(test_polynomial PRIVATE pencil_rank project_warnings testing)
add_test(NAME polynomial COMMAND test_polynomial)

add_executable(test_pencil_rank test_pencil_rank.cpp)
target_link_libraries(test_pencil_rank PRIVATE pencil_rank project_warnings testing)
add_test(NAME pencil_rank COMMAND test_pencil_rank ${PROJECT_SOURCE_DIR}/evidence/fixtures)

# The independent check against exhaustive search, and the only test here that
# costs anything: it settles the same six tensors by exhaustion, which is
# exponential where the formula is not.
add_executable(test_pencil_agreement test_pencil_agreement.cpp)
target_link_libraries(test_pencil_agreement PRIVATE pencil_rank greedy_heuristic exhaustive
                                                    project_warnings testing)
add_test(NAME pencil_agreement COMMAND test_pencil_agreement ${PROJECT_SOURCE_DIR}/evidence/fixtures)
set_tests_properties(pencil_agreement PROPERTIES LABELS slow TIMEOUT 900)

add_executable(test_projection_lower_bound test_projection_lower_bound.cpp)
target_link_libraries(test_projection_lower_bound PRIVATE pencil_rank project_warnings testing)
add_test(NAME projection_lower_bound
         COMMAND test_projection_lower_bound ${PROJECT_SOURCE_DIR}/evidence/fixtures)
