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
