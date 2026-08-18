# Exact rank for two slices, in polynomial time. The one module here that does
# not search: the answer is read off a canonical form.
add_library(pencil_rank STATIC polynomial.cpp prime_power_factors.cpp pencil_divisors.cpp
                               minimal_indices.cpp kronecker_structure.cpp)
target_include_directories(pencil_rank PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pencil_rank PUBLIC linear_algebra formats PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
