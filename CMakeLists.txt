cmake_minimum_required(VERSION 3.22)
project(tensor_rank_toolkit VERSION 0.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# `cmake --install build --prefix ~/.local` puts the thirteen tools on PATH,
# which is the assumption every documented command line makes when it types
# `minimise-rank` bare. Each tool's install() sits beside its add_executable.
# The three instruments (`measure-leaf`, `price-canonical-route`,
# `show-limits`) and the `list-solvers` shim stay in the build tree: an
# instrument prints facts about one machine and one working directory, so it
# belongs beside the build it measures rather than on a PATH that outlives it.
include(GNUInstallDirs)

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

add_subdirectory(infrastructure/cli)
add_subdirectory(infrastructure/testing)
add_subdirectory(infrastructure/run_limits)
add_subdirectory(linear_algebra)
add_subdirectory(formats)
add_subdirectory(methods/bilinear_rank/map_construction)
add_subdirectory(methods/bilinear_rank/greedy_heuristic)
# Before exhaustive, because `decide-rank` asks it whether a two-slice
# tensor is settled in polynomial time before it builds any pool.
add_subdirectory(pencil_rank)
add_subdirectory(methods/bilinear_rank/exhaustive)
add_subdirectory(methods/bilinear_rank/orbit_reduction)
add_subdirectory(methods/bilinear_rank/flip_graph)
# After greedy_heuristic, whose minimum-weight basis it grows, and before nothing:
# the upper-bound search links the heuristic and is linked by no other module.
add_subdirectory(methods/bilinear_rank/branch_and_bound)
add_subdirectory(integer_programme)
add_subdirectory(matrix_sparsification)
add_subdirectory(satisfiability)
add_subdirectory(curve_bounds)
add_subdirectory(rank_metric_bound)
add_subdirectory(methods/bilinear_rank/canonical_augmentation)
add_subdirectory(canonical_factorisation)
# After both of the modules whose enums a plan names, and after run_limits,
# whose device ranking it routes: search_plan owns no rule and links every
# module that owns one.
add_subdirectory(methods/bilinear_rank/search_plan)

# Read unconditionally, and guarded inside instead. infrastructure/gpu_leaf/README.md is a
# measurement that stands on its own and the directory builds nothing the rest of
# the repository links, so a machine without nvcc still loses a number rather
# than a capability. What it must not lose is the compiler: guarded out here,
# `measure_leaf.cpp` spent 2026-08-20 failing to compile against a constructor
# that had gained an argument, on every machine, with CI green throughout. A
# directory nothing compiles is a directory that drifts.
add_subdirectory(infrastructure/gpu_leaf)
