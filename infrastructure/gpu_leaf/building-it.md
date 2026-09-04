# Building the kernels, which is not the obvious command

`nvcc` is not on `PATH` here. It lives in the `cuda` conda environment, which
the machine's tooling notes record as the reason no `nvidia-cuda-toolkit`
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

The binary that build produces is real and runnable, confirmed here without
starting a measurement, `--help` printed exactly as captured:

    $ ./build-cuda/gpu_leaf/measure-leaf --help
    # usage: measure-leaf [check|measure|both|floor] [shipped-rows] [packed-rows]
    #
    #   check    every device answer against the host's (the default)
    #   measure  the scan, the walk and the widest walk, timed
    #   both     check, then measure
    #   floor    where the card starts winning, re-fit
    #   --help   print this and stop, as exit 2

**`-DCMAKE_CUDA_ARCHITECTURES=89` was on that line and is gone from it.**
`CMakeLists.txt` now asks CMake for `native` instead of the RTX 4060 pin that
made every other card fail silently; why, and what it cost, is in
[`../run_limits/adapting-to-the-machine/fitted-or-genuine.md`](../run_limits/adapting-to-the-machine/fitted-or-genuine.md#fitted-and-there-were-four-two-are-now-derived).
Pass the flag only to build for a card that is *not* here; an explicit value
still wins, which is what a packager cross-compiling needs.
