add_executable(test_flip_graph test_flip_graph.cpp)
target_link_libraries(test_flip_graph PRIVATE flip_graph exhaustive formats
                                              testing project_warnings)
add_test(NAME flip_graph COMMAND test_flip_graph ${PROJECT_SOURCE_DIR}/evidence/fixtures)
