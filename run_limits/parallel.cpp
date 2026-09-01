#include "parallel.h"

#include <atomic>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

#include "machine.h"

namespace run_limits {

namespace {

std::size_t workers = 1;

}  // namespace

std::size_t worker_count() { return workers; }

void set_worker_count(std::size_t requested) {
    if (requested != 0) {
        workers = requested;
        return;
    }
    workers = core_count();
}

void parallel_for(std::size_t count, const std::function<void(std::size_t)>& body) {
    const std::size_t threads = worker_count() < count ? worker_count() : count;
    if (threads <= 1) {
        for (std::size_t index = 0; index < count; ++index) body(index);
        return;
    }

    // One shared counter rather than a range each: the work per index is wildly
    // uneven, so whoever finishes first takes the next.
    std::atomic<std::size_t> next(0);
    std::vector<std::thread> pool;
    pool.reserve(threads - 1);

    // **What a worker throws is carried out rather than dropped on the floor.**
    // An exception that leaves a `std::thread`'s function calls `std::terminate`,
    // so before this every `require_room` refusal raised under `--threads 2` or
    // more was a SIGABRT instead of the sentence naming the number — and the
    // refusal exists precisely so that a machine smaller than the one a run was
    // written on degrades rather than dies. The first one wins and the rest are
    // dropped, because a caller wants the reason and not `threads` copies of it;
    // `stop` makes the others give up their remaining indices, so a refused run
    // ends at once instead of finishing the work it has already been told it
    // cannot afford.
    std::atomic<bool> stop(false);
    std::mutex first_failure;
    std::exception_ptr failure;

    const auto run = [&] {
        for (;;) {
            if (stop.load(std::memory_order_relaxed)) return;
            const std::size_t index = next.fetch_add(1);
            if (index >= count) return;
            try {
                body(index);
            } catch (...) {
                const std::lock_guard<std::mutex> only_the_first(first_failure);
                if (!failure) failure = std::current_exception();
                stop.store(true, std::memory_order_relaxed);
                return;
            }
        }
    };
    for (std::size_t worker = 0; worker + 1 < threads; ++worker) pool.emplace_back(run);
    run();  // this thread works too, rather than waiting on the others
    for (std::thread& worker : pool) worker.join();
    if (failure) std::rethrow_exception(failure);
}

}  // namespace run_limits
