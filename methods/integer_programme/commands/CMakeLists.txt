# As in the other strands: the source moved into commands/, the binary did not.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# Not a command any more, and it no longer links this module: what it prints is
# the line to type instead. `curve-bounds --solvers` is where the ranking is, and
# is what still links integer_programme.
add_executable(list-solvers solver_report.cpp)
target_link_libraries(list-solvers PRIVATE cli project_warnings)
