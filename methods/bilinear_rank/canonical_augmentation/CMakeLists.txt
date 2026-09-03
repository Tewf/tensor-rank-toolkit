# The oracle answers one question; this decides which questions are worth asking.
# One file per job: answering one `k` with a proof instead of a budget, buying that
# proof from the tree rather than from a solver, and generating each subspace once
# per orbit rather than once per orbit element. Naming a subspace left for
# `orbit_reduction` on 2026-08-23, where a second consumer that cannot link this
# module can reach it; `subspace_canon.h` still resolves, through that module's
# PUBLIC include directory.
#
# Two files left for the `rejected-experiments` branch: the fixed-`k` finder,
# whose assumed asymmetry between acceptance and refutation measured as about one,
# and the descending sweep, which existed to hand `find_rank` a bracket and was
# measured to lose on all seven fixtures once the rank sums closed the gap it was
# meant to close. `measurements/` keeps both sets of numbers.
add_library(canonical_augmentation STATIC strict_deflation.cpp tree_refutation.cpp
                                        factored_lex_min.cpp factored_set_stabiliser.cpp
                                        pool_set_canon.cpp
                                        canonical_parent.cpp
                                        canonical_augmentation.cpp canonical_route_price.cpp
                                        pool_cosets.cpp)
target_include_directories(canonical_augmentation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# This module is where the two sides meet, and the meeting is one-directional:
# nothing in `satisfiability` or `orbit_reduction` knows this exists. That is the
# same arrangement `satisfiability/commands` documents, moved into a library so a
# test can link it.
target_link_libraries(canonical_augmentation PRIVATE permlib
                                          PUBLIC satisfiability orbit_reduction
                                                  greedy_heuristic exhaustive
                                                  run_limits
                                           PRIVATE map_construction cli project_warnings)

# The prices `canonical_route_price.cpp` above decides from, measured rather than
# guessed. It is here and not in `commands/` because it is an instrument and not
# a tool: what it prints is nanoseconds, and `../MEASURING.md`'s line is that
# counts reproduce anywhere and timings do not. `measure-leaf` sits exactly here
# relative to `device_launch_floor`, which is the precedent the file's own header
# cites. A constant whose measurement cannot be repeated is a constant nobody can
# move when the machine changes, so it ships beside the predicate either way.
add_executable(price-canonical-route price_canonical_route.cpp)
target_link_libraries(price-canonical-route PRIVATE canonical_augmentation cli formats
                                                    project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
