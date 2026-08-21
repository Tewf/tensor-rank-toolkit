set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# `deflate-strictly` exists to prove candidates impossible, which is the half of
# the fixed-`k` question that survived the finder's removal: an upper bound is
# cheaper from the descent, and a refutation is not available from it at all.
add_executable(deflate-strictly deflate_strictly_main.cpp)
target_link_libraries(deflate-strictly PRIVATE oracle_guided_search cli formats project_warnings)

# The enumerator is its own command because its question is a count, not a bound:
# how many solution subspaces are there, and how many once the group is quotiented
# out.
add_executable(enumerate-subspaces enumerate_subspaces_main.cpp)
target_link_libraries(enumerate-subspaces PRIVATE oracle_guided_search cli formats
                                                  project_warnings)

# The prices `canonical_route_price.h` decides from, measured rather than guessed.
# It ships beside the predicate for the same reason `measure-leaf` ships beside
# `device_launch_floor`: a constant whose measurement cannot be repeated is a
# constant nobody can move when the machine changes.
add_executable(price-canonical-route price_canonical_route_main.cpp)
target_link_libraries(price-canonical-route PRIVATE oracle_guided_search cli formats
                                                    project_warnings)
