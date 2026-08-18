set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# --symmetry quotients the pool and --plateau walks across equal-cost schemes, so
# this command reaches for the two folders built on top of the heuristic. The
# library needs neither.
add_executable(minimise-rank minimise_rank_main.cpp)
target_link_libraries(minimise-rank PRIVATE descent_search exhaustive_search
                                            orbit_reduction flip_graph formats cli
                                            project_warnings)
