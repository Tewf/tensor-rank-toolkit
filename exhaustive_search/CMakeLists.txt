# The complete decision: is there an algorithm with exactly k products? An
# implementation of a published algorithm, exponential, so it settles small maps
# outright and bounds large ones from below.
add_library(exhaustive_search STATIC exhaustive_search.cpp fewest_products.cpp
                                    rank_one_basis.cpp subspace_walk.cpp gf2_leaf.cpp)
target_include_directories(exhaustive_search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# `descent_search` for `candidate_pool.h`, which owns `RankOnePool` and the two
# accessor shims both searches walk their candidates through.
#
# The dependency is the wrong way round and is recorded rather than hidden: a
# pool of rank-one maps is not a descent's concept, and four modules now use it,
# so by the repository's own import-proximity rule it belongs at a common
# ancestor. That would be `linear_algebra`, which is header-only by design and
# cannot host `candidate_pool.cpp`. Moving it needs that decision revisited, so
# it joins the layering items in `positioning/`.
target_link_libraries(exhaustive_search PUBLIC linear_algebra run_limits descent_search
                                        PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
