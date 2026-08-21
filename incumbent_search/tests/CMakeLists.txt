# The admissibility of the bound and the completeness of the generated move set,
# both brute-forced against the whole rank-one pool on the fixtures small enough
# to afford it, and the two answers the search is expected to reach.
add_executable(test_cost_first_search test_cost_first_search.cpp)
target_link_libraries(test_cost_first_search PRIVATE incumbent_search formats testing
                                                     project_warnings)
add_test(NAME cost_first_search COMMAND test_cost_first_search ${PROJECT_SOURCE_DIR}/fixtures)
