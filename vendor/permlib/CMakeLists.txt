# PermLib, header-only and third-party, attached as an INTERFACE target.
#
# SYSTEM on the include directory is deliberate: this repository compiles at
# -Wall -Wextra -Wpedantic and PermLib is 2011-era code that still names
# std::binary_function. Those warnings are upstream's and not ours to carry, and
# the alternative is patching a vendored tree, which would end its usefulness as
# something diffable against upstream.
add_library(permlib INTERFACE)
target_include_directories(permlib SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
