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
