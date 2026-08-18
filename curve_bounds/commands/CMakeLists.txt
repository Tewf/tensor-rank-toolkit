set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# The command is where the two routes meet, so it is the command and not the table
# that links a solver. curve_bounds itself stays free of third-party headers.
add_executable(curve-bounds curve_bounds_main.cpp)
target_link_libraries(curve-bounds PRIVATE curve_bounds_by_solver project_warnings)
