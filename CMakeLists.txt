cmake_minimum_required(VERSION 3.22)
project(tensor_rank_toolkit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ccache when the machine has it, and nothing when it does not, so this changes
# how long a build takes and never what it produces. Guarded on find_program
# rather than required, which keeps Givaro the only dependency.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Givaro, from the CASYS team at the LJK, for exact arithmetic over GF(p) and
# over the rationals. Both strands here search over ranks and over counts of
# nonzero entries, so an answer that is nearly right answers a different
# question; nothing in this repository is ever a float.
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIVARO REQUIRED IMPORTED_TARGET givaro)

add_library(project_warnings INTERFACE)
# Guarded on the language rather than left bare: nvcc rejects these for device
# code, and a genex belongs here where compile options are legal rather than in
# a `target_link_libraries` call, where CMake refuses it outright.
target_compile_options(project_warnings
                       INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>)

enable_testing()
# Third-party and header-only, so it comes before anything that links it.
# vendor/permlib/README.md says what it is and why it is not nauty.
add_subdirectory(vendor/permlib)

add_subdirectory(cli)
add_subdirectory(testing)
add_subdirectory(run_limits)
add_subdirectory(linear_algebra)
add_subdirectory(formats)
add_subdirectory(map_construction)
add_subdirectory(descent_search)
add_subdirectory(exhaustive_search)
add_subdirectory(orbit_reduction)
add_subdirectory(flip_graph)
add_subdirectory(integer_programme)
add_subdirectory(matrix_sparsification)
add_subdirectory(satisfiability)
add_subdirectory(curve_bounds)
add_subdirectory(rank_metric_bound)
add_subdirectory(oracle_guided_search)
add_subdirectory(pencil_rank)
add_subdirectory(canonical_factorisation)

# Only where there is a card to measure. gpu_leaf/README.md is the measurement
# and stands on its own; the directory builds nothing the rest of the repository
# links, so a machine without nvcc loses a number and not a capability.
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
  add_subdirectory(gpu_leaf)
endif()
