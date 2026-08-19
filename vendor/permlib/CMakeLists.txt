# PermLib, header-only and third-party, attached as an INTERFACE target.
#
# SYSTEM on the include directory is deliberate: this repository compiles at
# -Wall -Wextra -Wpedantic and PermLib is 2011-era code that still names
# std::binary_function. Those warnings are upstream's and not ours to carry, and
# the alternative is patching a vendored tree, which would end its usefulness as
# something diffable against upstream.
add_library(permlib INTERFACE)
target_include_directories(permlib SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# PermLib's point type is `unsigned short` unless this is defined, so a degree
# past 65 535 wraps and writes out of bounds. It does not refuse and it does not
# slow down: it segfaults, inside `Transversal::foundOrbitElement`, with a healthy
# stack. `<3,3,3>`'s pool is 261 121 points, so this repository is on the wrong
# side of that ceiling by a factor of four and would have concluded the library
# does not scale. It scales; the default type does not. Costs four bytes a point
# instead of two.
target_compile_definitions(permlib INTERFACE PERMLIB_DOMAIN_INT)
