# Building the bilinear maps every method is then run on.
add_library(map_construction STATIC map_construction.cpp)
target_include_directories(map_construction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(map_construction PUBLIC linear_algebra run_limits
                                       PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
