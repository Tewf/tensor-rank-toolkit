/// What the worker pool does with the machine it is on, and with a body that
/// throws.
///
/// **The count is asked of the machine and never of a table.**
/// `set_worker_count(0)` is the whole of axis (a): a run on a machine with more
/// cores than the one this was written on gets more workers, and one with fewer
/// gets fewer, with no number from any chassis in between. This asserts that
/// against `hardware_concurrency` rather than against 12, because 12 is what this
/// laptop has and a test that said 12 would be a test of the laptop.
///
/// **A body that throws is carried out to the caller.** This is the one that was
/// wrong. `require_room` exists so that a machine smaller than the one a run was
/// written on says which allocation it cannot afford instead of being killed —
/// and above one worker that refusal left a `std::thread`, which is
/// `std::terminate`, so the one graceful failure here became the one abrupt one
/// and only when `--threads` was given. A test that only ran the pool at one
/// worker would not have seen it, so every case below is run at four as well.
///
/// The pool is exercised at a count far above the worker count and at counts
/// below it, since `parallel_for` clamps the threads to the work and the
/// one-worker path is a different branch from the pooled one.
#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "check.h"
#include "parallel.h"

namespace {

using run_limits::parallel_for;
using run_limits::set_worker_count;
using run_limits::worker_count;

/// Every index exactly once, whatever the worker count.
long long visits_that_were_not_one(std::size_t count, std::size_t workers) {
    set_worker_count(workers);
    std::vector<std::atomic<int>> seen(count);
    for (std::atomic<int>& slot : seen) slot.store(0);
    parallel_for(count, [&](std::size_t index) { seen[index].fetch_add(1); });

    long long wrong = 0;
    for (const std::atomic<int>& slot : seen) wrong += slot.load() == 1 ? 0 : 1;
    return wrong;
}

/// The message a body's exception arrives with, or a sentence saying it did not
/// arrive at all. Returned rather than asserted here so the failure prints what
/// happened instead of what was expected.
std::string what_escaped(std::size_t count, std::size_t workers, std::size_t throwing) {
    set_worker_count(workers);
    try {
        parallel_for(count, [&](std::size_t index) {
            if (index == throwing) throw std::runtime_error("the body refused");
        });
    } catch (const std::runtime_error& refusal) {
        return refusal.what();
    } catch (...) {
        return "something that is not a runtime_error";
    }
    return "nothing: parallel_for returned normally";
}

}  // namespace

int main() {
    check::equal("one worker by default", static_cast<long long>(worker_count()), 1);

    // The machine's own answer, whatever machine that is. `hardware_concurrency`
    // may report 0 where it cannot tell, which `set_worker_count` turns into 1.
    const unsigned int detected = std::thread::hardware_concurrency();
    set_worker_count(0);
    check::equal("nought asks the machine and gets what it has",
                 static_cast<long long>(worker_count()),
                 static_cast<long long>(detected == 0 ? 1u : detected));
    check::equal("which is at least one wherever this runs",
                 static_cast<long long>(worker_count() >= 1), 1);

    set_worker_count(3);
    check::equal("and a count given is the count used", static_cast<long long>(worker_count()), 3);

    check::equal("every index once on one worker", visits_that_were_not_one(1000, 1), 0);
    check::equal("every index once on four", visits_that_were_not_one(1000, 4), 0);
    check::equal("and once when the workers outnumber the work",
                 visits_that_were_not_one(2, 8), 0);

    // The four cases that used to be two different failures: one worker
    // propagated, more than one aborted the process.
    check::text("a throwing body on one worker reaches the caller",
                what_escaped(50, 1, 0), "the body refused");
    check::text("a throwing body on four workers reaches the caller too",
                what_escaped(50, 4, 0), "the body refused");
    check::text("wherever in the range it throws", what_escaped(50, 4, 49),
                "the body refused");
    check::text("and when the workers outnumber the work", what_escaped(2, 8, 1),
                "the body refused");

    // A body that does not throw still runs to the end after one that did: the
    // stop flag is per call and not per process.
    check::equal("a later call is not poisoned by an earlier refusal",
                 visits_that_were_not_one(1000, 4), 0);

    set_worker_count(1);  // process-wide: leave it as it was
    return check::report("parallel");
}
