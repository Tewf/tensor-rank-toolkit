# The one branch of the backend no question in this repository reaches, forced.
#
# It exists only where the toolkit does, so a machine without nvcc runs the same
# suite it always ran and this is one test more rather than one fixture more.
# `measure-leaf check` covers the agreement on every shape; this covers what
# happens when the buffer that agreement rests on is too small.
add_executable(test_survivor_overflow test_survivor_overflow.cpp)
target_include_directories(test_survivor_overflow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(test_survivor_overflow PRIVATE gpu_leaf_kernels descent_search
                                                     linear_algebra testing project_warnings)
add_test(NAME survivor_overflow COMMAND test_survivor_overflow)
set_tests_properties(survivor_overflow PROPERTIES TIMEOUT 300)
