set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

# The way in from the published ecosystem: reads the (L,R,P) triple the
# field publishes, by algorithm_recovery.h's convention, which is why it
# lives beside that header and not in core/formats/.
add_executable(operators-to-tensor operators_to_tensor_main.cpp)
target_link_libraries(operators-to-tensor PRIVATE bilinear_rank formats cli
                                                  project_warnings)

install(TARGETS operators-to-tensor)
