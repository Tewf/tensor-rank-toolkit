# As in the other strand: the source moved, the binary did not.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(sparsify-operator sparsify_main.cpp)
target_link_libraries(sparsify-operator PRIVATE matrix_sparsification sparsify_by_simplex
                                                cli project_warnings)
