add_executable(test_canonical_factorisation test_canonical_factorisation.cpp)
target_link_libraries(test_canonical_factorisation PRIVATE canonical_factorisation
                                                           project_warnings testing)
add_test(NAME canonical_factorisation
         COMMAND test_canonical_factorisation ${PROJECT_SOURCE_DIR}/evidence/fixtures)

# Slow, and the reason is the finding: the parent test names orbits the
# pre-image way, walking 216 group elements per node, the stage
# canonical-augmentation.md's history retired. Labelled rather than dropped,
# because a route that ships has to be known to still agree.
add_executable(test_canonical_route test_canonical_route.cpp)
target_link_libraries(test_canonical_route PRIVATE canonical_factorisation project_warnings
                                                  testing)
add_test(NAME canonical_route COMMAND test_canonical_route ${PROJECT_SOURCE_DIR}/evidence/fixtures)
set_tests_properties(canonical_route PROPERTIES LABELS slow TIMEOUT 900)
