add_executable(test_curve_bounds test_curve_bounds.cpp)
target_link_libraries(test_curve_bounds PRIVATE curve_bounds project_warnings testing)
add_test(NAME curve_bounds COMMAND test_curve_bounds)

# The test that makes either route trustworthy: the same minimisation enumerated
# and solved, over a sweep, with the built-in forced because it is the only
# backend whose optimum is a proof. It links the solver target, which is why it is
# a second binary rather than more cases in the first: test_curve_bounds stays
# free of third-party headers, like the library it checks.
add_executable(test_interpolation_by_solver test_interpolation_by_solver.cpp)
target_link_libraries(test_interpolation_by_solver PRIVATE curve_bounds_by_solver
                                                          project_warnings testing)
add_test(NAME interpolation_by_solver COMMAND test_interpolation_by_solver)
set_tests_properties(interpolation_by_solver PROPERTIES TIMEOUT 300)
