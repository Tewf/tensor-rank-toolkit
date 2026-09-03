# Header-only: every operation is templated on the field, so both strands share
# one implementation instead of one per element type.
add_library(linear_algebra INTERFACE)
target_include_directories(linear_algebra INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# `rank_metric_bound` is INTERFACE too, and the two require each other:
# `rank_lower_bound.h` takes its Griesmer term, and that module's own bounds read
# the contraction-rank table from here. CMake accepts a mutual INTERFACE
# dependency in either declaration order, which was tested rather than assumed.
#
# `run_limits` joins them because `tensor_rank_sum.h` spreads its rank table over
# cores. It is the one thing in this header-only library that owns a loop rather
# than answering one call, so it is the one thing here that has to know how much
# of the machine a run was given.
target_link_libraries(linear_algebra INTERFACE PkgConfig::GIVARO rank_metric_bound run_limits)

add_subdirectory(tests)
