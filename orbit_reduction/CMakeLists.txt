# A change of coordinates on each operand preserves rank, so if it also fixes
# the target subspace it maps solutions to solutions and only one member of each
# orbit need be visited. The group, the orbits, and each consumer of them are
# separate jobs, which is why this is six files and not one.
add_library(orbit_reduction STATIC automorphism.cpp group_construction.cpp orbit_search.cpp
                                   orbit_heuristic.cpp pool_orbits.cpp orbit_cubes.cpp)
target_include_directories(orbit_reduction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# PUBLIC where a header of ours includes theirs: orbit_search.h reaches for
# exhaustive_search.h, so a consumer compiling against us needs it on its path.
# PRIVATE where only a .cpp does. A static library propagates even a private
# dependency as a link requirement, which is what is wanted here: the link
# works, the include directories stay off consumers' path.
target_link_libraries(orbit_reduction PUBLIC linear_algebra run_limits exhaustive_search
                                      PRIVATE descent_search map_construction
                                              project_warnings)

add_subdirectory(tests)
