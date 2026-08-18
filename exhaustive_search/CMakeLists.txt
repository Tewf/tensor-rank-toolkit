# The complete decision: is there an algorithm with exactly k products? An
# implementation of a published algorithm, exponential, so it settles small maps
# outright and bounds large ones from below.
add_library(exhaustive_search STATIC exhaustive_search.cpp fewest_products.cpp
                                    rank_one_basis.cpp)
target_include_directories(exhaustive_search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(exhaustive_search PUBLIC linear_algebra run_limits
                                        PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
