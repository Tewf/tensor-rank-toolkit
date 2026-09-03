# Header-only, like the bounds it sits beside: one templated function over the
# field, so both strands share one implementation. It reads the contraction-rank
# table `linear_algebra` already builds and adds nothing to the build.
add_library(rank_metric_bound INTERFACE)
target_include_directories(rank_metric_bound INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rank_metric_bound INTERFACE linear_algebra)

add_subdirectory(tests)
