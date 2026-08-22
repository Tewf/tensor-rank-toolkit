# The ranking behaves when the card is absent, which is every machine this builds
# on today. No fixtures: the seam knows nothing about tensors.
add_executable(test_device test_device.cpp)
target_link_libraries(test_device PRIVATE run_limits testing project_warnings)
add_test(NAME device COMMAND test_device)

# And the pool: that the worker count comes from the machine rather than from a
# number somebody measured here, and that a refusal thrown by a body reaches the
# caller instead of ending the process. The second is why this file exists.
add_executable(test_parallel test_parallel.cpp)
target_link_libraries(test_parallel PRIVATE run_limits testing project_warnings)
add_test(NAME parallel COMMAND test_parallel)

# The three limits reach the commands that need them, which no test above can
# see: every check here reads run_limits on both sides, and would pass through a
# period when no command offered --threads or --max-memory at all. So this one
# spends processes, as cli/ and search_plan/ do.
add_test(NAME limits_reach_the_commands
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_the_limits_reach_the_commands.sh
                 $<TARGET_FILE:decide-rank> ${PROJECT_SOURCE_DIR}/fixtures)
# 174 s idle on an i5-12450H, so 300 was a ceiling fitted to a quiet machine and
# this test is the one that exists to catch exactly that. It timed out at load
# 6.07 while three other things built. Labelled slow and given room, because a
# test that flakes under load teaches people to rerun rather than to read.
set_tests_properties(limits_reach_the_commands PROPERTIES TIMEOUT 1200 LABELS slow)

# And the probe: that the memory ceiling is derived from the machine rather than
# fitted to this one. The rule is checked as a pure function of a hypothetical
# scale, since a test asserting 2 GiB would be a test of this laptop.
add_executable(test_machine test_machine.cpp)
target_link_libraries(test_machine PRIVATE run_limits testing project_warnings)
add_test(NAME machine COMMAND test_machine)
