add_executable(test_formula_to_tensor test_formula_to_tensor.cpp)
target_link_libraries(test_formula_to_tensor PRIVATE satisfiability project_warnings testing)
add_test(NAME formula_to_tensor COMMAND test_formula_to_tensor)

add_executable(test_binary_encoding test_binary_encoding.cpp)
target_link_libraries(test_binary_encoding PRIVATE satisfiability project_warnings testing)
add_test(NAME binary_encoding COMMAND test_binary_encoding)

add_executable(test_prime_field_encoding test_prime_field_encoding.cpp)
target_link_libraries(test_prime_field_encoding PRIVATE satisfiability project_warnings testing)
add_test(NAME prime_field_encoding COMMAND test_prime_field_encoding)

add_executable(test_field_theory_encoding test_field_theory_encoding.cpp)
target_link_libraries(test_field_theory_encoding PRIVATE satisfiability project_warnings testing)
add_test(NAME field_theory_encoding COMMAND test_field_theory_encoding)

# A solver that can only find, asserted against a stub named for one so that a
# machine with none installed still checks the class; each real one on PATH is
# then asked the smallest fixture end to end.
add_executable(test_finds_only test_finds_only.cpp)
target_link_libraries(test_finds_only PRIVATE satisfiability project_warnings testing)
add_test(NAME finds_only COMMAND test_finds_only ${CMAKE_CURRENT_SOURCE_DIR}/stub_solvers
                                                 ${PROJECT_SOURCE_DIR}/fixtures)

add_executable(test_end_to_end test_end_to_end.cpp)
target_link_libraries(test_end_to_end PRIVATE satisfiability project_warnings testing)
add_test(NAME end_to_end COMMAND test_end_to_end ${PROJECT_SOURCE_DIR}/fixtures)

# The same loop on a tensor the solver has to work for, with the refusal checked
# by drat-trim. Labelled slow the way bilinear_rank labels its own searches.
add_test(NAME end_to_end_slow COMMAND test_end_to_end ${PROJECT_SOURCE_DIR}/fixtures --slow)
set_tests_properties(end_to_end_slow PROPERTIES LABELS slow TIMEOUT 900)

# The exit codes, asserted against the built commands rather than described in a
# document. One path goes in and the script finds the other commands beside it,
# since every strand's binary lands in the same build tree. Cheap: every case
# but the five-second SAT one answers in milliseconds.
add_test(NAME exit_codes
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_exit_codes.sh
                 $<TARGET_FILE:decide-rank-by-sat> ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(exit_codes PROPERTIES TIMEOUT 120)
add_executable(test_streamlining test_streamlining.cpp)
target_link_libraries(test_streamlining PRIVATE satisfiability project_warnings testing)
add_test(NAME streamlining COMMAND test_streamlining ${PROJECT_SOURCE_DIR}/fixtures)
