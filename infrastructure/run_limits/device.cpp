#include "device.h"

namespace run_limits {

namespace {

bool (*gpu_probe)() = nullptr;

/// Measured 2026-08-21 on this machine, an RTX 4060 Laptop against one core of
/// an i5-12450H, by `measure-leaf floor`.
///
/// **It is the smallest element count at which every compiled route won**, and
/// that is the whole rule: below it at least one route loses, so no question
/// below it is sent to a card. Both routes were swept on all four shapes a
/// kernel exists for, the card's column being `GpuSurvivors::wall_seconds`: the
/// launch, the kernel and the copy back, because the launch and the copy back
/// are precisely the two costs a floor exists to price. A walk row is a whole
/// subspace of its dimension rather than a prefix of a wider one, since the
/// kernel rebuilds an element from every basis row and a leaf never walks a
/// prefix; a scan row is whole rows of the outer-product grid, which is the
/// smallest unit either side works in.
///
/// | route | first count the card won |
/// |---|---|
/// | walk 16x16, 9x9, 4x4 | 4 096 |
/// | **walk 5x5** | **8 192**, and 4 096 was 0.93x |
/// | scan 9x9 | 8 176, and 4 088 was 0.57x |
/// | scan 16x16 | at or under 65 535, its one-row floor, at 5.6x |
/// | scan 5x5, 4x4 | never: their whole pools are 961 and 225 elements |
///
/// The card's fixed cost is 21-29 us of wall clock across every shape, and the
/// host is 3.1-4.0 ns an element, which is where 8 192 comes from and why the
/// four crossovers sit within a factor of two of each other. The 16x16 scan is
/// the one row that is a bound rather than a crossing: one row of that grid is
/// 65 535 elements and neither side can be asked for fewer. Its own two costs
/// put it near 8 700, which is the same neighbourhood and is not quoted as a
/// measurement because nothing measured it.
///
/// Between 4 096 and 8 192 three of the routes would win by 1.0x to 1.9x, and
/// [`../../MEASURING.md`](../../MEASURING.md) does not report ratios that small on
/// this chassis: 13% of thermal spread covers most of that band. So the floor
/// costs nothing anybody could measure and buys a rule that holds everywhere.
///
/// ## What it is worth wired in, which is a different question
///
/// The sweep above prices one leaf. A search is thousands of them and one CUDA
/// context, so the two numbers do not compose. Measured the same day on
/// `decide-rank`, fastest of three at loadavg 0.94:
///
/// | question | one core | `--device auto` |
/// |---|---|---|
/// | `matmul_3x3x3 --target 17 --node-limit 60 --leaf-route walk` | **0.0711 s** | 0.1120 s |
/// | the same at `--node-limit 600` | 0.3927 s | **0.1692 s** |
/// | `matmul_3x3x3 --target 23 --node-limit 300` | 0.3168 s | **0.1565 s** |
///
/// **The first row is the context and not the leaves.** Fitting the two walk
/// rows gives the host 0.596 ms a node against the card's 0.106 ms, and
/// intercepts 0.0354 s against 0.1056 s: a fixed **0.070 s** the card's run pays
/// once, which is the CUDA context. It buys itself back after about **143
/// nodes**, and this is a floor in nodes rather than in elements, so it is not
/// this number and does not belong in it.
std::size_t floor_elements = 8192;

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
