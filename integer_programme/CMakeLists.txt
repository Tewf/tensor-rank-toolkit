# Handing a minimisation to somebody else's solver: the model, the built-in
# simplex and branch and bound, the MPS writer, and the chain that tries the
# installed backends in turn.
add_library(integer_programme STATIC integer_programme.cpp standard_form.cpp simplex.cpp
                                     branch_and_bound.cpp mps_format.cpp external_solver.cpp
                                     solver_chain.cpp)
target_include_directories(integer_programme PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(integer_programme PUBLIC run_limits linear_algebra PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
