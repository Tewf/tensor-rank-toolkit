# The constructed maps are checked by running both searches over them, so this
# reaches for the two method folders. It is the one test that spans them.
add_executable(test_map_construction test_map_construction.cpp)
target_link_libraries(test_map_construction PRIVATE map_construction descent_search
                                                    exhaustive_search testing project_warnings)
add_test(NAME map_construction COMMAND test_map_construction)
