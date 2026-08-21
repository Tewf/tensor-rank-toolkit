# The same tree the exact search walks, cut by an incumbent instead of a target:
# an upper-bound search that hands back an algorithm whenever it is stopped.
add_library(incumbent_search STATIC level_lowering_moves.cpp cost_first_search.cpp)
target_include_directories(incumbent_search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# PUBLIC because the headers here are written in descent_search's vocabulary:
# `minimum_weight_basis`, `span_element_ranks` and the aliases come from there.
target_link_libraries(incumbent_search PUBLIC descent_search
                                       PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
