#include "device.h"

namespace run_limits {

namespace {

bool (*gpu_probe)() = nullptr;

/// PROVISIONAL. A CUDA launch is conventionally about ten microseconds and the
/// card measured 7.4e9 elements a second on the widest walk the search poses, so
/// a launch pays for itself somewhere near 74 000 elements. **Neither number was
/// taken on this machine**, and the second is for a kernel that is not wired in,
/// so this is a placeholder with its arithmetic shown rather than a measurement.
std::size_t floor_elements = 100000;

std::vector<Device>& order() {
    static std::vector<Device> ranked{Device::Gpu, Device::Cpu};
    return ranked;
}

}  // namespace

const char* name_of(Device device) {
    switch (device) {
        case Device::Gpu: return "gpu";
        case Device::Cpu: return "cpu";
    }
    return "?";
}

const std::vector<Device>& ranked_devices() { return order(); }

bool set_device_order(const std::vector<std::string>& names, std::string& unrecognised) {
    std::vector<Device> ranked;
    for (const std::string& name : names) {
        if (name == "gpu") {
            ranked.push_back(Device::Gpu);
        } else if (name == "cpu") {
            ranked.push_back(Device::Cpu);
        } else {
            unrecognised = name;
            return false;
        }
    }
    // An order with no host in it would leave a question unanswerable on a
    // machine with no card, so the host is appended rather than the order
    // refused: a ranking is a preference and never a requirement.
    bool has_host = false;
    for (const Device device : ranked) has_host = has_host || device == Device::Cpu;
    if (!has_host) ranked.push_back(Device::Cpu);

    order() = ranked;
    return true;
}

bool available(Device device) {
    if (device == Device::Cpu) return true;
    return gpu_probe != nullptr && gpu_probe();
}

void register_gpu_backend(bool (*probe)()) { gpu_probe = probe; }

std::size_t launch_floor() { return floor_elements; }
void set_launch_floor(std::size_t elements) { floor_elements = elements; }

Device chosen_device(std::size_t elements) {
    if (elements < floor_elements) return Device::Cpu;
    for (const Device device : order()) {
        if (available(device)) return device;
    }
    return Device::Cpu;
}

}  // namespace run_limits
