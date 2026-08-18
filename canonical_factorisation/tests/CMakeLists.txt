add_executable(test_canonical_factorisation test_canonical_factorisation.cpp)
target_link_libraries(test_canonical_factorisation PRIVATE canonical_factorisation
                                                           project_warnings testing)
add_test(NAME canonical_factorisation
         COMMAND test_canonical_factorisation ${PROJECT_SOURCE_DIR}/fixtures)
