# The group, the orbits, the rule that rejects a repeat, and each consumer of
# them are separate jobs, which is why this is seven files and not one; the
# mathematical rationale is `README.md`'s.
# `subspace_canon.cpp` came from `oracle_guided_search` on 2026-08-23. It names a
# subspace so that two can be compared and a whole orbit has one name, which is
# this module's sentence rather than that one's, and two consumers now sit on
# either side of it: the canonical augmentation above and `incumbent_search`'s
# span census below, which may not link upward to reach it.
add_library(orbit_reduction STATIC automorphism.cpp group_construction.cpp orbit_search.cpp
                                   isomorph_rejection.cpp orbit_heuristic.cpp pool_orbits.cpp
                                   orbit_cubes.cpp subspace_canon.cpp)
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
