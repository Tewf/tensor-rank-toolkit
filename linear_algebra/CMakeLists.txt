# Header-only: every operation is templated on the field, so both strands share
# one implementation instead of one per element type.
add_library(linear_algebra INTERFACE)
target_include_directories(linear_algebra INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# `rank_metric_bound` is INTERFACE too, and the two require each other:
# `rank_lower_bound.h` takes its Griesmer term, and that module's own bounds read
# the contraction-rank table from here. CMake accepts a mutual INTERFACE
# dependency in either declaration order, which was tested rather than assumed.
target_link_libraries(linear_algebra INTERFACE PkgConfig::GIVARO rank_metric_bound)

add_subdirectory(tests)
