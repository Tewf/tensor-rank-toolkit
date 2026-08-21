# The admissibility of the bound and the completeness of the generated move set,
# both brute-forced against the whole rank-one pool on the fixtures small enough
# to afford it, and the two answers the search is expected to reach.
add_executable(test_cost_first_search test_cost_first_search.cpp)
target_link_libraries(test_cost_first_search PRIVATE incumbent_search formats testing
                                                     project_warnings)
add_test(NAME cost_first_search COMMAND test_cost_first_search ${PROJECT_SOURCE_DIR}/fixtures)

# The quotiented move set against the whole one, on every fixture where both
# finish. A group that does not stabilise the node's span removes the move that
# would have paid and the run reports a worse cost with nothing to show for it,
# which is the one mistake nothing downstream catches.
add_executable(test_orbit_moves test_orbit_moves.cpp)
target_link_libraries(test_orbit_moves PRIVATE incumbent_search formats testing project_warnings)
add_test(NAME orbit_moves COMMAND test_orbit_moves ${PROJECT_SOURCE_DIR}/fixtures)
