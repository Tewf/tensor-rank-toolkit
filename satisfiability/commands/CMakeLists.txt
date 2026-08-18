set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# The command is the common ancestor of the two modules, which is what keeps the
# layering one-directional: `satisfiability` must not gain a dependency on
# `orbit_reduction`, because the encoder's contract with the cubes is that it
# knows nothing about where they came from.
add_executable(decide-rank-by-sat decide_rank_by_sat_main.cpp)
target_link_libraries(decide-rank-by-sat PRIVATE satisfiability orbit_reduction cli
                                                 formats project_warnings)
