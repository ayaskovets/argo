#include <wip/queues/lockless_mpmc_queue.hpp>

#include <benchmark/benchmark.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace benchmarks::queues {

namespace queues = wip::queues;

template <typename Value>
using queue_t = queues::lockless_mpmc_queue<
    Value, std::size_t, swiss::kDynamicExtent<std::size_t>,
    std::allocator<queues::detail::lockless_mpmc_queue_entry<
        Value, std::size_t, std::hardware_destructive_interference_size>>>;

BENCHMARK_TEMPLATE_T(queues_lockless_mpmc_queue, nonblocking_throughput, int,
                     queue_t<int>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 2})
    ->Args({1024, 1048576, 1, 4})
    ->Args({1024, 1048576, 4, 1})
    ->Args({1024, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_lockless_mpmc_queue, nonblocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 2})
    ->Args({1024, 1048576, 1, 4})
    ->Args({1024, 1048576, 4, 1})
    ->Args({1024, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_lockless_mpmc_queue, blocking_throughput, int,
                     queue_t<int>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 2})
    ->Args({1024, 1048576, 1, 4})
    ->Args({1024, 1048576, 4, 1})
    ->Args({1024, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_lockless_mpmc_queue, blocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1024, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1024, 1048576, 2, 2})
    ->Args({1024, 1048576, 1, 4})
    ->Args({1024, 1048576, 4, 1})
    ->Args({1024, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
