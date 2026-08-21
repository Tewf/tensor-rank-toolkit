# Building the kernels, which is not the obvious command

`nvcc` is not on `PATH` here. It lives in the `cuda` conda environment, which
`~/Desktop/localAI/tooling.md` records as the reason no `nvidia-cuda-toolkit`
was ever apt-installed. That makes the obvious recipe wrong three ways, and the
one this file replaced was wrong all three.

**Building it needs a second build directory, not a reconfigured one.** `nvcc`
here is in a conda environment, and activating that environment puts its own C++
compiler in front, which cannot find the system Givaro. So the host compiler is
pinned to the system one and only `nvcc` is taken from the environment:

    CUDA=$HOME/miniforge3/envs/cuda
    PATH=$CUDA/bin:$PATH PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig \
      cmake -S . -B build-cuda -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_CUDA_COMPILER=$CUDA/bin/nvcc \
        -DCMAKE_CUDA_HOST_COMPILER=/usr/bin/g++ \
        -DCUDAToolkit_ROOT=$CUDA
    cmake --build build-cuda -j 2 --target measure-leaf
    flock /tmp/bilinear-measure.lock ./build-cuda/gpu_leaf/measure-leaf both

`CUDAToolkit_ROOT` is what `find_package` needs; `CMAKE_CUDA_COMPILER` alone
leaves it unfound and the target is never defined. Verified 2026-08-20 with
nvcc 12.9 from conda-forge against Givaro 4.2.0 from the system.

**`-DCMAKE_CUDA_ARCHITECTURES=89` was on that line and is gone from it.** It is
the RTX 4060 and it was also written into `CMakeLists.txt`, so a build on any
other card produced a fatbin with no image the device could run — and the failure
is invisible, because `leaf_backend.cpp` catches it and lets the host answer.
`CMakeLists.txt` now asks CMake for `native`, which asks the driver what is
really in the machine. Pass the flag only to build for a card that is *not* here;
an explicit value still wins, which is what a packager cross-compiling needs.
