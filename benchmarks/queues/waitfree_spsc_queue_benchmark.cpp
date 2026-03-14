#include <wip/queues/waitfree_spsc_queue.hpp>

#include <benchmark/benchmark.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace benchmarks::queues {

namespace queues = wip::queues;
namespace utils = wip::utils;

template <typename Value>
using queue_t = queues::waitfree_spsc_queue<
    Value, std::size_t, utils::kDynamicExtent<std::size_t>,
    std::allocator<queues::detail::waitfree_spsc_queue_entry<
        Value, std::hardware_destructive_interference_size>>>;

BENCHMARK_TEMPLATE_T(queues_waitfree_spsc_queue, nonblocking_throughput, int,
                     queue_t<int>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_waitfree_spsc_queue, nonblocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_waitfree_spsc_queue, blocking_throughput, int,
                     queue_t<int>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_waitfree_spsc_queue, blocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
