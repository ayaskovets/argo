#include <wip/queues/lockfree_mpsc_queue.hpp>

#include <benchmark/benchmark.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace benchmarks::queues {

namespace queues = wip::queues;
namespace utils = wip::utils;

template <typename Value>
using queue_t = queues::lockfree_mpsc_queue<
    Value, std::size_t, utils::kDynamicExtent<std::size_t>,
    std::allocator<queues::detail::lockfree_mpsc_queue_entry<
        Value, std::hardware_destructive_interference_size>>>;

BENCHMARK_TEMPLATE_T(queues_lockfree_mpsc_queue, nonblocking_throughput, int,
                     queue_t<int>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 1})
    ->Args({1024, 1048576, 4, 1})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_lockfree_mpsc_queue, nonblocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 1})
    ->Args({1024, 1048576, 4, 1})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_lockfree_mpsc_queue, blocking_throughput, int,
                     queue_t<int>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 1})
    ->Args({1024, 1048576, 4, 1})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_lockfree_mpsc_queue, blocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 1})
    ->Args({1024, 1048576, 4, 1})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
