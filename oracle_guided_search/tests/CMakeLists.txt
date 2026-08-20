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

# The parent test asks one cell of the child whether it is the distinguished one,
# where it used to ask all of them and take the least. The two agree by an
# argument about how the doubled ground set orders its keys, and an argument about
# every cell is checked on every cell: the search's own sets at 2x2x2 and 2x2x3,
# and random subsets of the pool, which are sets no span produces.
add_executable(test_distinguished_cell test_distinguished_cell.cpp)
target_link_libraries(test_distinguished_cell PRIVATE oracle_guided_search formats testing
                                                      project_warnings)
add_test(NAME distinguished_cell COMMAND test_distinguished_cell ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(distinguished_cell PROPERTIES TIMEOUT 900)

# The other half of the same node: a candidate parent's pool content is kept from
# the membership tests that found it instead of being scanned for again, and the
# kept list has to be the scanned one exactly, on every parent the search visits.
add_executable(test_parent_pool_content test_parent_pool_content.cpp)
target_link_libraries(test_parent_pool_content PRIVATE oracle_guided_search formats testing
                                                       project_warnings)
add_test(NAME parent_pool_content
         COMMAND test_parent_pool_content ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(parent_pool_content PROPERTIES TIMEOUT 900)
