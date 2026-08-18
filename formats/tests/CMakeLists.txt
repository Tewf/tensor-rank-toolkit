add_executable(test_sms test_sms.cpp)
target_link_libraries(test_sms PRIVATE formats project_warnings testing)
add_test(NAME sms COMMAND test_sms ${PROJECT_SOURCE_DIR}/fixtures)

add_executable(test_sms_interoperability test_sms_interoperability.cpp)
target_link_libraries(test_sms_interoperability PRIVATE formats project_warnings testing)
add_test(NAME sms_interoperability
         COMMAND test_sms_interoperability ${PROJECT_SOURCE_DIR}/fixtures)

add_executable(test_tensor_file test_tensor_file.cpp)
target_link_libraries(test_tensor_file PRIVATE formats project_warnings testing)
add_test(NAME tensor_file COMMAND test_tensor_file)

add_executable(test_dimacs test_dimacs.cpp)
target_link_libraries(test_dimacs PRIVATE formats project_warnings testing)
add_test(NAME dimacs COMMAND test_dimacs)
