set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(make-tensor make_tensor_main.cpp)
target_link_libraries(make-tensor PRIVATE map_construction formats cli project_warnings)
