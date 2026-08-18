add_executable(test_symmetry_argument test_symmetry_argument.cpp)
target_link_libraries(test_symmetry_argument PRIVATE cli project_warnings testing)
add_test(NAME symmetry_argument COMMAND test_symmetry_argument)

add_executable(test_arguments test_arguments.cpp)
target_link_libraries(test_arguments PRIVATE cli project_warnings testing)
add_test(NAME arguments COMMAND test_arguments)

add_executable(test_report test_report.cpp)
target_link_libraries(test_report PRIVATE cli project_warnings testing)
add_test(NAME report COMMAND test_report)

# The repository root, because the last check compares the shipped tunables.conf
# against the defaults compiled into tunables.h, and those must not drift apart.
add_executable(test_tunables test_tunables.cpp)
target_link_libraries(test_tunables PRIVATE cli project_warnings testing)
add_test(NAME tunables COMMAND test_tunables ${PROJECT_SOURCE_DIR})
