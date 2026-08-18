# As in the other strands: the source moved, the binary did not.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(factor-over-canonical-basis factor_over_canonical_basis_main.cpp)
target_link_libraries(factor-over-canonical-basis PRIVATE canonical_factorisation cli
                                                          project_warnings)
