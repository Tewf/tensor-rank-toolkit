# What one consumer GPU is worth on the leaf test, measured rather than
# forecast. A proof of concept and not an integration: nothing outside this
# directory calls it, and the two searches are byte for byte what they were.
#
# Guarded on the toolkit rather than requiring it, so a machine without nvcc
# configures and builds the rest of the repository unchanged. The top-level
# CMakeLists.txt carries the same guard, so this file is only read where CUDA is
# present and `enable_language(CUDA)` will succeed.
# The harness's C++ half, compiled on every machine and linked on none of them.
# It is an object library rather than a target anybody runs: `measure_leaf.cpp`
# calls into the kernels, so it cannot link without them, but it can be
# type-checked without them — and that is the whole point. This is what would
# have caught `Gf2Leaf` gaining a `field` argument on 2026-08-20.
add_library(measure-leaf-host-check OBJECT measure_leaf.cpp)
target_include_directories(measure-leaf-host-check PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(measure-leaf-host-check PRIVATE exhaustive_search descent_search
                                                     linear_algebra run_limits project_warnings)

find_package(CUDAToolkit QUIET)
if(NOT CUDAToolkit_FOUND)
  message(STATUS "gpu_leaf: no CUDA toolkit, so the kernels are not built. "
                 "The harness's C++ is still compiled.")
  return()
endif()

enable_language(CUDA)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# The two kernels and the backend that offers them to the search, compiled once
# and linked by both the harness and the command. 8.9 is the RTX 4060 this was
# measured on. The ptxas report is how the register and local-memory counts in
# README.md were read off; a kernel that spills is measuring the memory system
# rather than the arithmetic, so the report is on by default.
#
# `project_warnings` carries -Wall -Wextra -Wpedantic behind a COMPILE_LANGUAGE
# guard, so linking it here warns the C++ and leaves nvcc's device compilation
# alone. The guard lives with the options because CMake refuses a
# COMPILE_LANGUAGE genex inside `target_link_libraries`.
add_library(gpu_leaf_kernels STATIC pool_scan.cu subspace_walk.cu leaf_backend.cpp)
target_include_directories(gpu_leaf_kernels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# `CUDA::cudart` is PUBLIC because both things that link this are ordinary C++
# executables now: CMake adds the runtime for a target that has `.cu` sources of
# its own and for no other, which is why it used to arrive by itself and does
# not any more.
target_link_libraries(gpu_leaf_kernels PUBLIC exhaustive_search CUDA::cudart
                                       PRIVATE project_warnings)
set_target_properties(gpu_leaf_kernels PROPERTIES CUDA_ARCHITECTURES "89")
target_compile_options(gpu_leaf_kernels PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--ptxas-options=-v>)

add_executable(measure-leaf measure_leaf.cpp)
target_link_libraries(measure-leaf PRIVATE gpu_leaf_kernels exhaustive_search descent_search
                                           linear_algebra run_limits project_warnings)

# The registration, and only the registration.
#
# An object library rather than a static one because a static library
# contributes only the objects something already references, and nothing
# references this by design: linking it *is* the registration. It is separate
# from the kernels above because `measure-leaf` must not have it — a harness
# whose shipped-leaf column had quietly started answering on the card would be
# comparing the card with itself.
add_library(gpu_leaf_registration OBJECT register_the_card.cpp)
target_include_directories(gpu_leaf_registration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gpu_leaf_registration PRIVATE gpu_leaf_kernels run_limits project_warnings)

# Reaching into `exhaustive_search/commands/` from here rather than guarding
# there: whether a card is compiled in is this directory's fact, and the command
# should not have to ask. Where there is no toolkit this file has already
# returned and `decide-rank` is byte for byte what it was.
target_link_libraries(decide-rank PRIVATE gpu_leaf_registration gpu_leaf_kernels)

add_subdirectory(tests)
