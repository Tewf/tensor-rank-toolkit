# The rank as a factorisation over the canonical basis, with the receipt that
# makes an answer checkable without rerunning the search that found it.
add_library(canonical_factorisation STATIC canonical_basis.cpp factorisation.cpp)
target_include_directories(canonical_factorisation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(canonical_factorisation PUBLIC linear_algebra formats descent_search
                                                     exhaustive_search orbit_reduction satisfiability cli
                      PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
