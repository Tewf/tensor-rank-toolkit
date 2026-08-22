add_library(matrix_sparsification STATIC combinations.cpp omega_validator.cpp
                                         greedy_sparsifier.cpp rational_sparsifier.cpp
                                         multivariate_polynomial.cpp pattern_feasibility.cpp
                                         algorithm_cost.cpp algorithm_check.cpp)
target_include_directories(matrix_sparsification PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(matrix_sparsification PUBLIC linear_algebra formats run_limits
                                            PRIVATE project_warnings)

# The simplex route is its own target, so `integer_programme/` does not land on
# the include path of everything that links the sparsifier. `curve_bounds` keeps
# its solver at arm's length the same way and for the same reason.
add_library(sparsify_by_simplex STATIC lightest_vector_by_simplex.cpp)
target_include_directories(sparsify_by_simplex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sparsify_by_simplex PUBLIC matrix_sparsification integer_programme
                                          PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
