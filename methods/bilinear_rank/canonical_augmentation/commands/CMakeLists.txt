set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# `decide-rank-by-deflation` exists to prove candidates impossible, which is the half of
# the fixed-`k` question that survived the finder's removal: an upper bound is
# cheaper from the descent, and a refutation is not available from it at all.
add_executable(decide-rank-by-deflation decide_rank_by_deflation_main.cpp)
target_link_libraries(decide-rank-by-deflation PRIVATE canonical_augmentation cli formats project_warnings)

# The enumerator is its own command because its question is a count, not a bound:
# how many solution subspaces are there, and how many once the group is quotiented
# out.
add_executable(enumerate-subspaces enumerate_subspaces_main.cpp)
target_link_libraries(enumerate-subspaces PRIVATE canonical_augmentation cli formats
                                                  project_warnings)

# `price-canonical-route` was here and is not a command: its output is
# nanoseconds. It is built one directory up, beside the predicate it prices,
# where `measure-leaf` already sits relative to `device_launch_floor`.

install(TARGETS decide-rank-by-deflation enumerate-subspaces)
