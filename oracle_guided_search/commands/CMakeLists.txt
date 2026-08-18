set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(find-at-rank find_at_rank_main.cpp)
target_link_libraries(find-at-rank PRIVATE oracle_guided_search cli formats project_warnings)

# Kept a separate command from the finder rather than a flag on it. They answer
# different questions: one produces an upper bound and never claims a lower one,
# the other exists to prove candidates impossible. A single command doing both
# would need an "and" to describe.
add_executable(deflate-strictly deflate_strictly_main.cpp)
target_link_libraries(deflate-strictly PRIVATE oracle_guided_search cli formats project_warnings)

# The enumerator is its own command because its question is a count, not a bound:
# how many solution subspaces are there, and how many once the group is quotiented
# out.
add_executable(enumerate-subspaces enumerate_subspaces_main.cpp)
target_link_libraries(enumerate-subspaces PRIVATE oracle_guided_search cli formats
                                                  project_warnings)
