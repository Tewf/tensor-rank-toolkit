set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# --symmetry quotients the pool and --plateau walks across equal-cost schemes, so
# this command reaches for the two folders built on top of the heuristic. The
# library needs neither.
add_executable(minimise-rank minimise_rank_main.cpp)
target_link_libraries(minimise-rank PRIVATE descent_search exhaustive_search
                                            orbit_reduction flip_graph formats cli
                                            project_warnings)

# The inverse of `minimise-rank --emit-operators`, so it sits beside it: one
# writes the ⟨L,R,P⟩ triple the surrounding ecosystem reads, the other reads the
# triple that ecosystem publishes. Both are `algorithm_recovery.h`'s convention,
# which is why neither belongs in formats/.
add_executable(operators-to-tensor operators_to_tensor_main.cpp)
target_link_libraries(operators-to-tensor PRIVATE descent_search formats cli project_warnings)

install(TARGETS minimise-rank operators-to-tensor)
