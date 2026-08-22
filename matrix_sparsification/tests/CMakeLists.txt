add_executable(test_sparsify test_sparsify.cpp)
target_link_libraries(test_sparsify PRIVATE matrix_sparsification project_warnings testing)
add_test(NAME sparsify COMMAND test_sparsify ${PROJECT_SOURCE_DIR}/fixtures)

add_executable(test_pattern_feasibility test_pattern_feasibility.cpp)
target_link_libraries(test_pattern_feasibility PRIVATE matrix_sparsification project_warnings testing)
add_test(NAME pattern_feasibility
         COMMAND test_pattern_feasibility ${PROJECT_SOURCE_DIR}/fixtures)

add_executable(test_algorithm_cost test_algorithm_cost.cpp)
target_link_libraries(test_algorithm_cost PRIVATE matrix_sparsification project_warnings testing)
add_test(NAME algorithm_cost COMMAND test_algorithm_cost ${PROJECT_SOURCE_DIR}/fixtures)

# An operator and its transpose reach the same count. They must, and until
# 2026-08-22 they did not: a wide operator went through every method then
# shipped unchanged
# because the question is vacuous on that side. Every fixture here is 7x4, which
# is why nothing caught it; this check writes the transpose itself.
add_test(NAME sparsify_orientation
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_orientation_does_not_decide.sh
                 $<TARGET_FILE:sparsify-operator> ${PROJECT_SOURCE_DIR}/fixtures)

# The exact method over a finite field, against brute force over every basis.
# The exactness is a theorem about the algorithm; this is about the code.
add_executable(test_finite_field_sparsifier test_finite_field_sparsifier.cpp)
target_link_libraries(test_finite_field_sparsifier
                      PRIVATE matrix_sparsification project_warnings testing)
add_test(NAME finite_field_sparsifier COMMAND test_finite_field_sparsifier)

# The exact method over Q, squeezed from both sides: a walk over every column
# subset with no ceiling, and every basis drawn from a small coefficient box.
# The exactness is a theorem about the algorithm; this is about the code.
add_executable(test_rational_sparsifier test_rational_sparsifier.cpp)
target_link_libraries(test_rational_sparsifier
                      PRIVATE matrix_sparsification project_warnings testing)
add_test(NAME rational_sparsifier COMMAND test_rational_sparsifier)

# What `--emit` writes is a drop-in for what it read: same shape, already minimal.
# The count is the same either way up and a program is not, so nothing about the
# reported number would have caught this.
add_test(NAME sparsify_emit
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_emitted_operator_is_a_drop_in.sh
                 $<TARGET_FILE:sparsify-operator> ${PROJECT_SOURCE_DIR}/fixtures)

# The simplex route against the exhaustive scan, where the scan can answer. The
# first continuous-only programme in this repository: every case in
# integer_programme's own battery sets `integral = true`, so nothing exercised
# the LP path until now.
add_executable(test_lightest_vector_by_simplex test_lightest_vector_by_simplex.cpp)
target_link_libraries(test_lightest_vector_by_simplex
                      PRIVATE sparsify_by_simplex project_warnings testing)
add_test(NAME lightest_vector_by_simplex COMMAND test_lightest_vector_by_simplex)

# Every route the command offers still answers, and none beats the proved
# minimum. Three flags changed meaning on 2026-08-22 and nothing exercised them;
# this also asserts that the flag which was removed is refused rather than
# ignored, which is how a stale script goes on looking correct.
add_test(NAME sparsify_routes
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_every_route_answers.sh
                 $<TARGET_FILE:sparsify-operator> ${PROJECT_SOURCE_DIR}/fixtures)
