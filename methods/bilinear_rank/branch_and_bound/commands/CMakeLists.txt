set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# `exhaustive` for `flattening_floor` and `require_bound_consistent`: the
# floor is the only thing printed beside the count that this search cannot prove
# itself, and the two commands must print the same one.
add_executable(tighten-rank-bound tighten_rank_bound_main.cpp)
target_link_libraries(tighten-rank-bound PRIVATE branch_and_bound exhaustive formats cli
                                              project_warnings)

install(TARGETS tighten-rank-bound)
