set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# --from k walks from the heuristic's scheme rather than the naive one, and
# refuses when the heuristic cannot reach k, which is what needs fewest_products.
add_executable(walk-scheme walk_scheme_main.cpp)
target_link_libraries(walk-scheme PRIVATE flip_graph exhaustive_search formats cli
                                          project_warnings)
