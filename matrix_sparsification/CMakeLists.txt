add_library(matrix_sparsification STATIC combinations.cpp heuristic_sparsifier.cpp
                                         oracle_sparsifier.cpp greedy_sparsifier.cpp
                                         multivariate_polynomial.cpp pattern_feasibility.cpp
                                         algorithm_cost.cpp algorithm_check.cpp)
target_include_directories(matrix_sparsification PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(matrix_sparsification PUBLIC linear_algebra formats run_limits
                                            PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
