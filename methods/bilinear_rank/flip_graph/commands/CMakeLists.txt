set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# --from k walks from the heuristic's scheme rather than the naive one, and
# refuses when the heuristic cannot reach k, which is what needs fewest_products.
add_executable(walk-scheme walk_scheme_main.cpp)
# `run_limits` for `parallel_for`: the seeds are independent walks, so this is the
# one search here whose workers share nothing at all.
target_link_libraries(walk-scheme PRIVATE flip_graph exhaustive formats cli run_limits
                                          project_warnings)

install(TARGETS walk-scheme)
