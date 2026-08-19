#pragma once

#include <cuda_runtime.h>

#include <stdexcept>
#include <string>

/// The CUDA runtime made to obey C++ scope: failures become exceptions and
/// resources free themselves.
///
/// The runtime reports by return code and by a sticky error the next call
/// inherits, so a kernel that never ran looks exactly like one that found
/// nothing. A measurement that cannot tell those apart is worse than no
/// measurement, which is why nothing here ignores a status. And once a status
/// can throw, every handle held across one has to release itself, or a run that
/// reports a shape it has no kernel for also leaks the card.
namespace gpu_leaf {

inline void check(cudaError_t status, const char* call) {
    if (status == cudaSuccess) return;
    throw std::runtime_error(std::string(call) + ": " + cudaGetErrorString(status));
}

#define GPU_LEAF_CHECK(call) ::gpu_leaf::check((call), #call)

/// Device memory that frees itself.
struct DeviceBuffer {
    void* pointer = nullptr;
    explicit DeviceBuffer(std::size_t bytes) { GPU_LEAF_CHECK(cudaMalloc(&pointer, bytes)); }
    ~DeviceBuffer() { cudaFree(pointer); }
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
};

/// A timing event that destroys itself, which matters because every launch
/// between two of them can throw.
struct Event {
    cudaEvent_t handle = nullptr;
    Event() { GPU_LEAF_CHECK(cudaEventCreate(&handle)); }
    ~Event() { cudaEventDestroy(handle); }
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
};

/// Seconds between two events, both of which must already have been recorded.
inline double seconds_between(const Event& opened, const Event& closed) {
    float milliseconds = 0.0f;
    GPU_LEAF_CHECK(cudaEventElapsedTime(&milliseconds, opened.handle, closed.handle));
    return milliseconds / 1000.0;
}

}  // namespace gpu_leaf
