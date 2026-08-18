add_executable(test_fixtures test_fixtures.cpp)
target_link_libraries(test_fixtures PRIVATE formats project_warnings testing)
add_test(NAME fixtures COMMAND test_fixtures ${PROJECT_SOURCE_DIR}/fixtures)

add_executable(test_rank_one test_rank_one.cpp)
target_link_libraries(test_rank_one PRIVATE formats project_warnings testing)
add_test(NAME rank_one COMMAND test_rank_one ${PROJECT_SOURCE_DIR}/fixtures)

# Two tests on one bound, written independently on the two branches and both
# kept: this one checks the bound against the ranks the fixtures are known to
# have, the next one checks it on tensors built to a rank chosen in advance.
add_executable(test_tensor_flattening test_tensor_flattening.cpp)
target_link_libraries(test_tensor_flattening PRIVATE formats project_warnings testing)
add_test(NAME tensor_flattening COMMAND test_tensor_flattening ${PROJECT_SOURCE_DIR}/fixtures)

add_executable(test_flattening test_flattening.cpp)
target_link_libraries(test_flattening PRIVATE formats project_warnings testing)
add_test(NAME flattening COMMAND test_flattening ${PROJECT_SOURCE_DIR}/fixtures)

# The stronger bound, checked against the known ranks and against a brute-force
# enumeration of itself.
add_executable(test_rank_sum test_rank_sum.cpp)
target_link_libraries(test_rank_sum PRIVATE formats project_warnings testing)
add_test(NAME rank_sum COMMAND test_rank_sum ${PROJECT_SOURCE_DIR}/fixtures)
