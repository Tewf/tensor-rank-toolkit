# See test_device.cpp for what this asserts. No fixtures: the seam knows
# nothing about tensors.
add_executable(test_device test_device.cpp)
target_link_libraries(test_device PRIVATE run_limits testing project_warnings)
add_test(NAME device COMMAND test_device)

# See test_parallel.cpp for what this asserts.
add_executable(test_parallel test_parallel.cpp)
target_link_libraries(test_parallel PRIVATE run_limits testing project_warnings)
add_test(NAME parallel COMMAND test_parallel)

# See check_the_limits_reach_the_commands.sh for why this test exists.
add_test(NAME limits_reach_the_commands
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_the_limits_reach_the_commands.sh
                 $<TARGET_FILE:decide-rank> ${PROJECT_SOURCE_DIR}/evidence/fixtures
                 ${CMAKE_BINARY_DIR})
# 174 s idle on an i5-12450H, so 300 was a ceiling fitted to a quiet machine and
# this test is the one that exists to catch exactly that. It timed out at load
# 6.07 while three other things built. Labelled slow and given room, because a
# test that flakes under load teaches people to rerun rather than to read.
set_tests_properties(limits_reach_the_commands PROPERTIES TIMEOUT 1200 LABELS slow)

# See test_machine.cpp for what this asserts.
add_executable(test_machine test_machine.cpp)
target_link_libraries(test_machine PRIVATE run_limits testing project_warnings)
add_test(NAME machine COMMAND test_machine)

# See check_show_limits_names_every_tunable.sh for what this asserts.
add_test(NAME show_limits_names_every_tunable
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_show_limits_names_every_tunable.sh
                 $<TARGET_FILE:show-limits> ${PROJECT_SOURCE_DIR}/infrastructure/cli/tunables.h)
