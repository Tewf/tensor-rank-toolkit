# The oracle answers one question; this decides which questions are worth asking.
# One file per job: answering one `k` with a proof instead of a budget, buying that
# proof from the tree rather than from a solver, naming a subspace, and generating
# each subspace once per orbit rather than once per orbit element.
#
# Two files left for the `rejected-experiments` branch: the fixed-`k` finder,
# whose assumed asymmetry between acceptance and refutation measured as about one,
# and the descending sweep, which existed to hand `find_rank` a bracket and was
# measured to lose on all seven fixtures once the rank sums closed the gap it was
# meant to close. `measurements/` keeps both sets of numbers.
add_library(oracle_guided_search STATIC strict_deflation.cpp tree_refutation.cpp
                                        subspace_canon.cpp canonical_parent.cpp
                                        canonical_augmentation.cpp)
target_include_directories(oracle_guided_search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# This module is where the two sides meet, and the meeting is one-directional:
# nothing in `satisfiability` or `orbit_reduction` knows this exists. That is the
# same arrangement `satisfiability/commands` documents, moved into a library so a
# test can link it.
target_link_libraries(oracle_guided_search PUBLIC satisfiability orbit_reduction
                                                  descent_search exhaustive_search
                                                  run_limits
                                           PRIVATE map_construction cli project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
