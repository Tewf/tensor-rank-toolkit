# The assertion helper every module's tests use. It lived in linear_algebra's
# own tests/ and was reached from six other modules by hand, so every test
# CMakeLists repeated the same target_include_directories line. It sits at the
# root for the same reason cli/timing.h does: it belongs to no one module.
add_library(testing INTERFACE)
target_include_directories(testing INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
