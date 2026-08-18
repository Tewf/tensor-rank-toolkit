# Header-only, and deliberately narrow: what every command shares and no strand
# owns. A clock, an exit-code convention, the argument grammar, the
# stdout/stderr split, and the tunables read from a file. Anything that answers
# a question belongs to the strand that owns the question, not here.
add_library(cli INTERFACE)
target_include_directories(cli INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(tests)
