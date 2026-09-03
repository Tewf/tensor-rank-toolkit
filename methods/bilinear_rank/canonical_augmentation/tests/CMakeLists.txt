# The only check that can catch a wrong canonical parent: 36 subspaces in 1 orbit is
# known independently, so the two passes have to return exactly those two numbers.
#
# Two minutes, and left in the default suite rather than labelled slow, because a
# wrong parent test loses solutions in silence and nothing downstream would notice.
# The plain pass is nearly all of the cost, since it walks 1.9 million nodes to reach
# the 36 the canonical pass reaches in 83.
add_executable(test_canonical_augmentation test_canonical_augmentation.cpp)
target_link_libraries(test_canonical_augmentation PRIVATE canonical_augmentation formats testing
                                                          project_warnings)
add_test(NAME canonical_augmentation
         COMMAND test_canonical_augmentation ${PROJECT_SOURCE_DIR}/evidence/fixtures)
set_tests_properties(canonical_augmentation PROPERTIES TIMEOUT 900)

# The reduction's correctness claim: naming an orbit from generators must induce
# the same partition as walking all 216 elements. 2x2x2 is the largest shape where
# the walk is available as an oracle, which is why the check lives at that size.
add_executable(test_pool_set_canon test_pool_set_canon.cpp)
target_link_libraries(test_pool_set_canon PRIVATE canonical_augmentation formats testing
                                                  project_warnings)
add_test(NAME pool_set_canon COMMAND test_pool_set_canon ${PROJECT_SOURCE_DIR}/evidence/fixtures)

# The parent test asks one cell of the child whether it is the distinguished one,
# where it used to ask all of them and take the least. The two agree by an
# argument about how the doubled ground set orders its keys, and an argument about
# every cell is checked on every cell: the search's own sets at 2x2x2 and 2x2x3,
# and random subsets of the pool, which are sets no span produces.
add_executable(test_distinguished_cell test_distinguished_cell.cpp)
target_link_libraries(test_distinguished_cell PRIVATE canonical_augmentation formats testing
                                                      project_warnings)
add_test(NAME distinguished_cell COMMAND test_distinguished_cell ${PROJECT_SOURCE_DIR}/evidence/fixtures)
set_tests_properties(distinguished_cell PROPERTIES TIMEOUT 900)

# The other half of the same node: a candidate parent's pool content is kept from
# the membership tests that found it instead of being scanned for again, and the
# kept list has to be the scanned one exactly, on every parent the search visits.
add_executable(test_parent_pool_content test_parent_pool_content.cpp)
target_link_libraries(test_parent_pool_content PRIVATE canonical_augmentation formats testing
                                                       project_warnings)
add_test(NAME parent_pool_content
         COMMAND test_parent_pool_content ${PROJECT_SOURCE_DIR}/evidence/fixtures)
set_tests_properties(parent_pool_content PROPERTIES TIMEOUT 900)

# The claim the axis presentation makes is not that it is a good canonical form but
# that it is the same one, so it is checked against the grid presentation it
# replaced rather than against a property. `flattened_canon.h` keeps that
# presentation computable on the shapes where a permutation of the grid still fits,
# which is why this test and no other links `permlib` directly.
add_executable(test_factored_canonisation test_factored_canonisation.cpp)
target_link_libraries(test_factored_canonisation PRIVATE canonical_augmentation formats
                                                         map_construction testing permlib
                                                         project_warnings)
add_test(NAME factored_canonisation COMMAND test_factored_canonisation)
set_tests_properties(factored_canonisation PROPERTIES TIMEOUT 1800)

# The check that catches a canonical form that is too coarse, which is the one
# failure the enumeration's own totals cannot: a coarser form gives a smaller node
# count, and so does a better one. Complete rather than sampled (every pair and
# every triple of the 225-element pool against the partition the 216 elements
# define), because a spot check on the sets a span produces is exactly the check a
# too-coarse form passes.
#
# Labelled slow for the 1.9 million triples, not for the group.
add_executable(test_orbit_separation test_orbit_separation.cpp)
target_link_libraries(test_orbit_separation PRIVATE canonical_augmentation formats
                                                    map_construction testing project_warnings)
add_test(NAME orbit_separation COMMAND test_orbit_separation)
set_tests_properties(orbit_separation PROPERTIES TIMEOUT 3600 LABELS slow)

# The half of the contract that survives past the shapes an oracle reaches: a name
# that moves when its set is moved by a group element is a name of nothing. It is
# the weak half on purpose, and it is the only one that runs at <3,3,3>.
add_executable(test_canon_invariance test_canon_invariance.cpp)
target_link_libraries(test_canon_invariance PRIVATE canonical_augmentation formats
                                                    map_construction testing project_warnings)
add_test(NAME canon_invariance COMMAND test_canon_invariance)
set_tests_properties(canon_invariance PROPERTIES TIMEOUT 1800)

# The same program at <4,4,4>, which is the point of the whole reduction and not a
# check of it: 4 294 836 225 cells, no reference, nothing to sweep, and the only
# question is whether the group can be presented and a set named at all. Separate
# and labelled slow so that a failure says which of the two it was.
add_test(NAME canon_invariance_at_4x4x4 COMMAND test_canon_invariance --at-4x4x4)
set_tests_properties(canon_invariance_at_4x4x4 PROPERTIES TIMEOUT 3600 LABELS slow)

# The other question the presentation answers, held to the same standard: the
# subgroup the returned generators generate, against the subgroup the 216 elements
# define, as sets of permutations rather than as orders.
add_executable(test_stabiliser_order test_stabiliser_order.cpp)
target_link_libraries(test_stabiliser_order PRIVATE canonical_augmentation formats
                                                    map_construction testing project_warnings)
add_test(NAME stabiliser_order COMMAND test_stabiliser_order)
set_tests_properties(stabiliser_order PROPERTIES TIMEOUT 1800)

# The price model against the ten sweeps it was corrected on. No clock runs here:
# the seconds are constants measured once by hand, and what is checked is that the
# arithmetic still agrees with them. This predicate has no caller yet, so a change
# that quietly stopped agreeing would otherwise go unnoticed.
add_executable(test_route_price test_route_price.cpp)
target_link_libraries(test_route_price PRIVATE canonical_augmentation testing project_warnings)
add_test(NAME route_price COMMAND test_route_price)

# The whole of `PoolCosets`'s correctness: a child's content read off the cosets
# has to be the list the pool scan returns, element for element and in order, on
# every child of every subspace tried. GF(3) and GF(5) are in it because over GF(2)
# the residue's scalar class is trivial and a dropped normalisation would pass.
add_executable(test_pool_cosets test_pool_cosets.cpp)
target_link_libraries(test_pool_cosets PRIVATE canonical_augmentation formats testing
                                               project_warnings)
add_test(NAME pool_cosets COMMAND test_pool_cosets ${PROJECT_SOURCE_DIR}/evidence/fixtures)
set_tests_properties(pool_cosets PROPERTIES TIMEOUT 900)
