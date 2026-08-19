# The only check that can catch a wrong canonical parent: 36 subspaces in 1 orbit is
# known independently, so the two passes have to return exactly those two numbers.
#
# Two minutes, and left in the default suite rather than labelled slow, because a
# wrong parent test loses solutions in silence and nothing downstream would notice.
# The plain pass is nearly all of the cost, since it walks 1.9 million nodes to reach
# the 36 the canonical pass reaches in 954.
add_executable(test_canonical_augmentation test_canonical_augmentation.cpp)
target_link_libraries(test_canonical_augmentation PRIVATE oracle_guided_search formats testing
                                                          project_warnings)
add_test(NAME canonical_augmentation
         COMMAND test_canonical_augmentation ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(canonical_augmentation PROPERTIES TIMEOUT 900)

# The reduction's correctness claim: naming an orbit from generators must induce
# the same partition as walking all 216 elements. 2x2x2 is the largest shape where
# the walk is available as an oracle, which is why the check lives at that size.
add_executable(test_pool_set_canon test_pool_set_canon.cpp)
target_link_libraries(test_pool_set_canon PRIVATE oracle_guided_search formats testing
                                                  project_warnings)
add_test(NAME pool_set_canon COMMAND test_pool_set_canon ${PROJECT_SOURCE_DIR}/fixtures)
