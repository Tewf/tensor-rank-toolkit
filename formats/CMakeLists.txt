add_library(formats STATIC tensor_file.cpp dense_matrix_file.cpp sms_file.cpp dimacs_file.cpp smtlib_file.cpp)
target_include_directories(formats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(formats PUBLIC linear_algebra PRIVATE project_warnings)
add_subdirectory(tests)
