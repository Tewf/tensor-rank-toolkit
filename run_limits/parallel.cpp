#include "parallel.h"

#include <atomic>
#include <thread>
#include <vector>

namespace bilinear_rank {

namespace {

std::size_t workers = 1;

}  // namespace

std::size_t worker_count() { return workers; }

void set_worker_count(std::size_t requested) {
    if (requested != 0) {
        workers = requested;
        return;
    }
    const unsigned int detected = std::thread::hardware_concurrency();
    workers = detected == 0 ? 1 : detected;
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

    const auto run = [&] {
        for (;;) {
            const std::size_t index = next.fetch_add(1);
            if (index >= count) return;
            body(index);
        }
    };
    for (std::size_t worker = 0; worker + 1 < threads; ++worker) pool.emplace_back(run);
    run();  // this thread works too, rather than waiting on the others
    for (std::thread& worker : pool) worker.join();
}

}  // namespace bilinear_rank
