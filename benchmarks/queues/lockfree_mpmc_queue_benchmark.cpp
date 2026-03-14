#include <wip/queues/lockfree_mpmc_queue.hpp>

#include <benchmark/benchmark.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace benchmarks::queues {

namespace queues = wip::queues;
namespace utils = wip::utils;

template <typename Value>
using queue_t = queues::lockfree_mpmc_queue<
    Value, std::size_t, utils::kDynamicExtent<std::size_t>,
    std::allocator<std::atomic<
        queues::detail::lockfree_mpmc_queue_entry<Value, std::size_t>>>>;

BENCHMARK_T(queues_lockfree_mpmc_queue, nonblocking_throughput, queue_t<int>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 2})
    ->Args({1024, 1048576, 1, 4})
    ->Args({1024, 1048576, 4, 1})
    ->Args({1024, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_T(queues_lockfree_mpmc_queue, blocking_throughput, queue_t<int>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 2})
    ->Args({1024, 1048576, 1, 4})
    ->Args({1024, 1048576, 4, 1})
    ->Args({1024, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
