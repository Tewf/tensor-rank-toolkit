# The ranking behaves when the card is absent, which is every machine this builds
# on today. No fixtures: the seam knows nothing about tensors.
add_executable(test_device test_device.cpp)
target_link_libraries(test_device PRIVATE run_limits testing project_warnings)
add_test(NAME device COMMAND test_device)
