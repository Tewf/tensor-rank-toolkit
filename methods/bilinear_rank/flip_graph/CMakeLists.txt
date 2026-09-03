# Moving a decomposition that already works instead of building one: the flip
# graph of [kauers2023], and a walk allowed to cross plateaus.
add_library(flip_graph STATIC flip_graph.cpp plateau_search.cpp)
target_include_directories(flip_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Both are PUBLIC because both are header-level: flip_graph.h includes
# algorithm_recovery.h, plateau_search.h includes automorphism.h.
target_link_libraries(flip_graph PUBLIC greedy_heuristic orbit_reduction
                                 PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
