# What one consumer GPU is worth on the leaf test, measured rather than
# forecast. A proof of concept and not an integration: nothing outside this
# directory calls it, and the two searches are byte for byte what they were.
#
# Guarded on the toolkit rather than requiring it, so a machine without nvcc
# configures and builds the rest of the repository unchanged. The top-level
# CMakeLists.txt carries the same guard, so this file is only read where CUDA is
# present and `enable_language(CUDA)` will succeed.
enable_language(CUDA)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

add_executable(measure-leaf pool_scan.cu subspace_walk.cu measure_leaf.cpp)
target_include_directories(measure-leaf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# `project_warnings` carries -Wall -Wextra -Wpedantic behind a COMPILE_LANGUAGE
# guard, so linking it here warns the C++ and leaves nvcc's device compilation
# alone. The guard lives with the options because CMake refuses a
# COMPILE_LANGUAGE genex inside `target_link_libraries`.
target_link_libraries(measure-leaf PRIVATE exhaustive_search descent_search linear_algebra
                                           run_limits project_warnings)

# 8.9 is the RTX 4060 this was measured on. `-lineinfo` and the ptxas report are
# how the register and local-memory counts in README.md were read off; a kernel
# that spills is measuring the memory system rather than the arithmetic, so the
# report is on by default and the number is quoted there.
set_target_properties(measure-leaf PROPERTIES CUDA_ARCHITECTURES "89")
target_compile_options(measure-leaf PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--ptxas-options=-v>)
