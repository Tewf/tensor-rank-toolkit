# The checks that need no solver, which is the floor refusal and the arithmetic.
add_executable(test_fixed_rank_finder test_fixed_rank_finder.cpp)
target_link_libraries(test_fixed_rank_finder PRIVATE oracle_guided_search formats testing
                                                     project_warnings)
add_test(NAME fixed_rank_finder COMMAND test_fixed_rank_finder ${PROJECT_SOURCE_DIR}/fixtures)

# The one that calibrates the whole module: Strassen's seven, found and multiplied
# out, and a descent that stops at seven rather than below it. Behind a flag
# because it runs a solver.
add_test(NAME fixed_rank_finder_finds_strassen
         COMMAND test_fixed_rank_finder ${PROJECT_SOURCE_DIR}/fixtures --with-solver)
set_tests_properties(fixed_rank_finder_finds_strassen PROPERTIES LABELS slow TIMEOUT 900)

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
