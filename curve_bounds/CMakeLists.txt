# The published table, and the exact enumeration that spends it. No third-party
# header reaches this target: it is arithmetic on small integers and nothing else.
# run_limits is not an exception to that: memory_budget.h is <cstddef> and
# <string>, and the frontier here is quadratic in --degree, so it has to be
# priced by the same budget every other bulk allocation is.
add_library(curve_bounds STATIC symmetric_bound_table.cpp interpolation_programme.cpp)
target_include_directories(curve_bounds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(curve_bounds PUBLIC run_limits PRIVATE project_warnings)

# The same minimisation as a model for a solver, in a target of its own so
# that integer_programme/ and Givaro stay off the include path of everything
# that links curve_bounds. Why a static library forces this split:
# interpolation_by_solver.h, commit 169a152.
add_library(curve_bounds_by_solver STATIC interpolation_by_solver.cpp)
target_link_libraries(curve_bounds_by_solver PUBLIC curve_bounds integer_programme
                                            PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
