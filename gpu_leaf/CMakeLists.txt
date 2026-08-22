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

# **Whether anybody chose an architecture, asked before CMake answers for them.**
#
# `enable_language(CUDA)` writes `CMAKE_CUDA_ARCHITECTURES` into the cache itself
# whenever nobody set it, from nvcc's own default: "If the user did not set
# CMAKE_CUDA_ARCHITECTURES, use the compiler's default" in
# `CMakeDetermineCUDACompiler.cmake`. A cache variable is `DEFINED`, so a guard
# asked after that point always answers yes, and the choice below was never made
# once. This is the only moment the two cases can be told apart, so the answer is
# taken here and read further down.
if(DEFINED CMAKE_CUDA_ARCHITECTURES OR DEFINED ENV{CUDAARCHS})
  set(gpu_leaf_architectures_chosen TRUE)
else()
  set(gpu_leaf_architectures_chosen FALSE)
endif()

enable_language(CUDA)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# The three kernels and the two backends that offer them, compiled once and
# linked by both the harness and the commands. Two leaf routes and one that
# ranks every element of a span, which is a different question for a different
# caller: `span_ranks.cu` answers `descent_search`'s and the other two answer
# `exhaustive_search`'s. The ptxas report is how the register and local-memory
# counts in README.md were read off; a kernel that spills is measuring the memory
# system rather than the arithmetic, so the report is on by default.
#
# `project_warnings` carries -Wall -Wextra -Wpedantic behind a COMPILE_LANGUAGE
# guard, so linking it here warns the C++ and leaves nvcc's device compilation
# alone. The guard lives with the options because CMake refuses a
# COMPILE_LANGUAGE genex inside `target_link_libraries`.
add_library(gpu_leaf_kernels STATIC pool_scan.cu subspace_walk.cu leaf_backend.cpp
                                    span_ranks.cu span_ranks_backend.cpp)
target_include_directories(gpu_leaf_kernels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# `CUDA::cudart` is PUBLIC because both things that link this are ordinary C++
# executables now: CMake adds the runtime for a target that has `.cu` sources of
# its own and for no other, which is why it used to arrive by itself and does
# not any more.
target_link_libraries(gpu_leaf_kernels PUBLIC exhaustive_search CUDA::cudart
                                       PRIVATE project_warnings)
# **Which card, asked of the machine rather than written down.**
#
# This said `"89"`, which is the RTX 4060 every number in README.md was measured
# on and nothing else. A build on any other card — 8.0 on an A100, 8.6 on a 3090,
# 9.0 on an H100, anything newer — produced a fatbin with no image the device
# could run, so every launch failed with `cudaErrorNoKernelImageForDevice`,
# `answered_or_the_host` in leaf_backend.cpp caught it as designed, and the run
# was **silently several hundred times slower** with one line on stderr to say
# so. A number fitted to one laptop is not a constant, and shipping it as one is
# a bug on somebody else's hardware.
#
# `native` asks the installed driver what is actually in the machine, which is
# the same question `card_present` asks at run time and the only one whose answer
# is right everywhere. It needs CMake 3.24; below that `all-major` builds one
# image per major architecture, which is larger and slower to compile but runs.
# A packager cross-compiling for hardware that is not present sets
# `CMAKE_CUDA_ARCHITECTURES`, or exports `CUDAARCHS`, and is left alone: the
# property is only written where nobody has chosen, so an explicit choice always
# wins over this guess. **Whether anybody chose is read from the flag captured
# above `enable_language(CUDA)` and not from the variable**, which by this line
# CMake has filled in from nvcc's default on every build that did not choose.
if(NOT gpu_leaf_architectures_chosen)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    set(gpu_leaf_architectures native)
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
    set(gpu_leaf_architectures all-major)
  else()
    # 3.22 has neither keyword, and 3.22 is what cmake_minimum_required allows.
    # One image per major architecture, written out: Pascal through Ada, which is
    # every card CUDA 11 and 12 both support.
    set(gpu_leaf_architectures 60 70 75 80 86 89)
  endif()
  set_target_properties(gpu_leaf_kernels PROPERTIES
                        CUDA_ARCHITECTURES "${gpu_leaf_architectures}")
  message(STATUS "gpu_leaf: CUDA architectures '${gpu_leaf_architectures}'. "
                 "Set CMAKE_CUDA_ARCHITECTURES to build for a card this machine does not have.")
endif()
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

# Reaching into the command directories from here rather than guarding in each
# of them: whether a card is compiled in is this directory's fact, and a command
# should not have to ask. Where there is no toolkit this file has already
# returned and all four are byte for byte what they were.
#
# Four and not one because there are two seams now. `decide-rank` is the one that
# reaches a leaf; the other three call `span_element_ranks`, directly or through
# the descent they start with, so the span seam is offered a card exactly where
# something asks it for ranks. Linking the registration into a command that never
# ranks a span would cost a CUDA context and buy nothing.
foreach(command decide-rank minimise-rank lower-the-bound walk-scheme)
  target_link_libraries(${command} PRIVATE gpu_leaf_registration gpu_leaf_kernels)
endforeach()

add_subdirectory(tests)
