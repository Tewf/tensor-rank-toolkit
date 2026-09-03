# The same tree the exact search walks, cut by an incumbent instead of a target:
# an upper-bound search that hands back an algorithm whenever it is stopped.
add_library(branch_and_bound STATIC level_lowering_moves.cpp cost_first_search.cpp
                                    orbit_moves.cpp span_census.cpp)
target_include_directories(branch_and_bound PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# PUBLIC because the headers here are written in greedy_heuristic's vocabulary:
# `minimum_weight_basis`, `span_element_ranks` and the aliases come from there.
#
# `orbit_reduction` is PUBLIC for the same reason: `cost_first_search.h` names
# `Automorphism` in the signature that takes a group to quotient by, so a
# consumer compiling against this header needs that one on its path.
target_link_libraries(branch_and_bound PUBLIC greedy_heuristic orbit_reduction
                                       PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
