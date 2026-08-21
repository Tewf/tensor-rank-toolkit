set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# `exhaustive_search` for `flattening_floor` and `require_bound_consistent`: the
# floor is the only thing printed beside the count that this search cannot prove
# itself, and the two commands must print the same one.
add_executable(lower-the-bound lower_the_bound_main.cpp)
target_link_libraries(lower-the-bound PRIVATE incumbent_search exhaustive_search formats cli
                                              project_warnings)
