# As in the other strands: the source moved, the binary did not.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(decide-rank-by-pencil decide_rank_by_pencil_main.cpp)
target_link_libraries(decide-rank-by-pencil PRIVATE pencil_rank cli project_warnings)
