# As in the other strands: the source moved into commands/, the binary did not.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(list-solvers solver_report.cpp)
target_link_libraries(list-solvers PRIVATE integer_programme project_warnings)
