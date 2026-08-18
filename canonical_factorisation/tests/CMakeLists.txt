add_executable(test_canonical_factorisation test_canonical_factorisation.cpp)
target_link_libraries(test_canonical_factorisation PRIVATE canonical_factorisation
                                                           project_warnings testing)
add_test(NAME canonical_factorisation
         COMMAND test_canonical_factorisation ${PROJECT_SOURCE_DIR}/fixtures)

# Slow, and the reason is the finding: the parent test walks 216 group elements
# per node, so it trades 16.2x fewer nodes for 15x the wall clock. Labelled
# rather than dropped, because a route that ships has to be known to still agree.
add_executable(test_canonical_route test_canonical_route.cpp)
target_link_libraries(test_canonical_route PRIVATE canonical_factorisation project_warnings
                                                  testing)
add_test(NAME canonical_route COMMAND test_canonical_route ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(canonical_route PROPERTIES LABELS slow TIMEOUT 900)
