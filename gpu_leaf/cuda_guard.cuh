#pragma once

#include <cuda_runtime.h>

#include <stdexcept>
#include <string>

/// A failed CUDA call becomes an exception naming the call.
///
/// The runtime reports by return code and by a sticky error the next call
/// inherits, so a kernel that never ran looks exactly like one that found
/// nothing. A measurement that cannot tell those apart is worse than no
/// measurement, which is why nothing here ignores a status.
namespace gpu_leaf {

inline void check(cudaError_t status, const char* call) {
    if (status == cudaSuccess) return;
    throw std::runtime_error(std::string(call) + ": " + cudaGetErrorString(status));
}

#define GPU_LEAF_CHECK(call) ::gpu_leaf::check((call), #call)

}  // namespace gpu_leaf
