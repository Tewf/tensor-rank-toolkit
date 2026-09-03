add_executable(test_symmetry_argument test_symmetry_argument.cpp)
target_link_libraries(test_symmetry_argument PRIVATE cli project_warnings testing)
add_test(NAME symmetry_argument COMMAND test_symmetry_argument)

add_executable(test_arguments test_arguments.cpp)
target_link_libraries(test_arguments PRIVATE cli project_warnings testing)
add_test(NAME arguments COMMAND test_arguments)

add_executable(test_report test_report.cpp)
target_link_libraries(test_report PRIVATE cli project_warnings testing)
add_test(NAME report COMMAND test_report)

# Why this one forks is in the file's own header comment. It is also the only
# caller `interrupt_cleanup.h` has: the scratch files it exists for are named in
# satisfiability/solver_process.cpp, which is where the registration belongs and
# is not this strand's to edit.
add_executable(test_interrupt_cleanup test_interrupt_cleanup.cpp)
target_link_libraries(test_interrupt_cleanup PRIVATE cli project_warnings testing)
add_test(NAME interrupt_cleanup COMMAND test_interrupt_cleanup)

# The repository root, because the last check compares the shipped tunables.conf
# against the defaults compiled into tunables.h, and those must not drift apart.
add_executable(test_tunables test_tunables.cpp)
target_link_libraries(test_tunables PRIVATE cli project_warnings testing)
add_test(NAME tunables COMMAND test_tunables ${PROJECT_SOURCE_DIR})

# That the commands read a line the way the walker says, which neither test above
# can see, for the reason the script's own header comment gives. This one spends
# processes.
add_test(NAME argument_grammar
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_argument_grammar.sh
                 $<TARGET_FILE:decide-rank> ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(argument_grammar PROPERTIES TIMEOUT 120)

# That a file bounds a real run, which the check above cannot see, for the reason
# its own header comment gives. This one spends a process, the way satisfiability/
# asserts the exit codes against the built commands rather than describing them.
add_test(NAME tunables_bound_a_run
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_tunables_bound_a_run.sh
                 $<TARGET_FILE:decide-rank> ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(tunables_bound_a_run PROPERTIES TIMEOUT 120)
