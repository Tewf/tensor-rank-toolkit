# Header-only: every operation is templated on the field, so both strands share
# one implementation instead of one per element type.
add_library(linear_algebra INTERFACE)
target_include_directories(linear_algebra INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(linear_algebra INTERFACE PkgConfig::GIVARO)

add_subdirectory(tests)
