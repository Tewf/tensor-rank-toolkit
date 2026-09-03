/// The device ranking, asked with no backend registered.
///
/// Four things are asserted, and three of them are about what happens when the
/// card is absent, because that is every machine without `nvcc` and a seam that
/// misbehaves when empty is worse than no seam. `infrastructure/gpu_leaf/` registers a real one
/// where the toolkit is found, and this test links `run_limits` alone, which is
/// what keeps it asking the empty question rather than a machine-dependent one.
///
/// **The host always answers.** An order naming only devices this build cannot
/// reach would leave a question unanswerable, so the host is appended rather than
/// the order refused: a ranking is a preference and never a requirement.
///
/// **A name that is not a device is refused with the name**, the way
/// `ilp_backend_order` is, so `tunables.conf` saying `tpu` stops the run rather
/// than quietly leaving the compiled order in force.
///
/// **Work below the launch floor stays here whatever the ranking says**, since a
/// launch costs more than the work, and that has to hold even once a backend
/// exists or the floor is decoration.
///
/// **The probe is asked afresh every time.** A cached answer would make a card
/// that disappeared between runs look present, which is the failure mode that
/// would be blamed on the search rather than on this.
#include <string>
#include <vector>

#include "check.h"
#include "device.h"

namespace {

bool present = false;
bool probe() { return present; }

}  // namespace

int main() {
    using run_limits::Device;

    check::text("the compiled order leads with the card",
                run_limits::name_of(run_limits::ranked_devices().front()), "gpu");
    check::equal("the host is always available",
                 static_cast<long long>(run_limits::available(Device::Cpu)), 1);
    check::equal("the card is not, with nothing registered",
                 static_cast<long long>(run_limits::available(Device::Gpu)), 0);
    check::text("so a large question still goes somewhere",
                run_limits::name_of(run_limits::chosen_device(1'000'000'000)), "cpu");

    std::string unrecognised;
    check::equal("an unknown device is refused",
                 static_cast<long long>(run_limits::set_device_order({"tpu"}, unrecognised)), 0);
    check::text("and named in the refusal", unrecognised, "tpu");

    check::equal("an order without the host is accepted",
                 static_cast<long long>(run_limits::set_device_order({"gpu"}, unrecognised)), 1);
    check::text("with the host appended, so nothing is unanswerable",
                run_limits::name_of(run_limits::ranked_devices().back()), "cpu");

    run_limits::register_gpu_backend(&probe);
    present = true;
    run_limits::set_launch_floor(100000);
    check::text("a large question reaches a card once one is registered",
                run_limits::name_of(run_limits::chosen_device(1'000'000)), "gpu");
    check::text("a small one does not, because a launch costs more than the work",
                run_limits::name_of(run_limits::chosen_device(99'999)), "cpu");

    present = false;
    check::text("and a card that goes away is noticed rather than cached",
                run_limits::name_of(run_limits::chosen_device(1'000'000)), "cpu");

    return check::report("device");
}
