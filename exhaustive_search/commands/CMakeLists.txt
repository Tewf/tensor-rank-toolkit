set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# --anchor heuristic runs the heuristic first and searches from its result and
# --symmetry quotients the tree, so this command needs three of the four method
# folders; the library needs none of the others.
add_executable(decide-rank decide_rank_main.cpp)
target_link_libraries(decide-rank PRIVATE exhaustive_search descent_search pencil_rank
                                          orbit_reduction search_plan formats cli
                                          project_warnings)
