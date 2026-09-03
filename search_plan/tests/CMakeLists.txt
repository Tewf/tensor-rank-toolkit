# The rules stated outright, rather than inferred from a node count three
# modules away, and the file that has to carry all seven of them intact. No
# fixtures: a plan is decided from sizes, and the sizes are written down here.
add_executable(test_search_plan test_search_plan.cpp)
target_link_libraries(test_search_plan PRIVATE search_plan testing project_warnings)
add_test(NAME search_plan COMMAND test_search_plan)

# And that the plan a run prints is the plan it carried out, which the check
# above cannot see: both its sides come from search_plan.h, so it would pass
# throughout a period when no command read a plan at all. This one spends
# processes, the way infrastructure/cli/ asserts its tunables against the built command.
add_test(NAME plan_reaches_a_run
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_the_plan_reaches_a_run.sh
                 $<TARGET_FILE:decide-rank> ${PROJECT_SOURCE_DIR}/fixtures)
set_tests_properties(plan_reaches_a_run PROPERTIES TIMEOUT 300)
